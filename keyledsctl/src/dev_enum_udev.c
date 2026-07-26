/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <libudev.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "dev_enum.h"
#include "keyleds.h"
#include "logging.h"

static bool fill_info_structure(struct udev_device * usbdev, struct udev_device * hiddev,
                                struct dev_enum_item * out)
{
    const char * str;

    if ((str = udev_device_get_devnode(hiddev)) == NULL) { return false; }
    out->path = malloc((strlen(str) + 1) * sizeof(char));
    if (out->path == NULL) { return false; }
    strcpy(out->path, str);

    if ((str = udev_device_get_sysattr_value(usbdev, "idVendor")) == NULL) {
        goto err_fill_free_path;
    }
    out->vendor_id = strtoul(str, NULL, 16);

    if ((str = udev_device_get_sysattr_value(usbdev, "idProduct")) == NULL) {
        goto err_fill_free_path;
    }
    out->product_id = strtoul(str, NULL, 16);

    str = udev_device_get_sysattr_value(usbdev, "serial");
    if (str == NULL) {
        out->serial = NULL;
    } else {
        out->serial = malloc((strlen(str) + 1) * sizeof(char));
        if (out->serial == NULL) { goto err_fill_free_path; }
        strcpy(out->serial, str);
    }
    out->description = NULL;

    return true;

err_fill_free_path:
    free(out->path);
    return false;
}


bool enum_find_by_serial(const char * serial, struct dev_enum_item ** out)
{
    struct udev * context;
    struct udev_enumerate * enumerator;
    struct udev_list_entry * dev_first, * dev_current;
    const char * syspath;
    struct udev_device * usbdev = NULL, * hiddev = NULL;
    bool result = false;

    assert(serial != NULL);
    assert(out != NULL);

    if ((context = udev_new()) == NULL) { return false; }

    /* Scan for usb device matching serial */
    if ((enumerator = udev_enumerate_new(context)) == NULL) {
        goto err_enum_free_context;
    }
    udev_enumerate_add_match_sysattr(enumerator, "serial", serial);

    if (udev_enumerate_scan_devices(enumerator) < 0) {
        goto err_find_free_enumerator;
    }
    if ((dev_first = udev_enumerate_get_list_entry(enumerator)) == NULL) {
        goto err_find_free_enumerator;
    }

    syspath = udev_list_entry_get_name(dev_first);
    if ((usbdev = udev_device_new_from_syspath(context, syspath)) == NULL) {
        goto err_find_free_enumerator;
    }
    udev_enumerate_unref(enumerator);

    /* Search usb device for hidraw interfaces */
    if ((enumerator = udev_enumerate_new(context)) == NULL) {
        goto err_enum_free_context;
    }
    udev_enumerate_add_match_parent(enumerator, usbdev);
    udev_enumerate_add_match_subsystem(enumerator, "hidraw");

    if (udev_enumerate_scan_devices(enumerator) < 0) {
        goto err_find_free_enumerator;
    }
    if ((dev_first = udev_enumerate_get_list_entry(enumerator)) == NULL) {
        goto err_find_free_enumerator;
    }

    udev_list_entry_foreach(dev_current, dev_first) {
        Keyleds * device;
        const char * devnode;

        syspath = udev_list_entry_get_name(dev_current);
        if ((hiddev = udev_device_new_from_syspath(context, syspath)) == NULL) {
            goto err_find_free_enumerator;
        }
        if ((devnode = udev_device_get_devnode(hiddev)) == NULL) {
            udev_device_unref(hiddev);
            continue;
        }
        if ((device = keyleds_open(devnode, KEYLEDSCTL_APP_ID)) == NULL) {
            udev_device_unref(hiddev);
            continue;
        }
        keyleds_close(device);
        *out = malloc(sizeof(**out));
        if (*out == NULL || !fill_info_structure(usbdev, hiddev, *out)) {
            free(*out);
            *out = NULL;
            udev_device_unref(hiddev);
            goto err_find_free_enumerator;
        }
        udev_device_unref(hiddev);
        result = true;
        break;
    }

err_find_free_enumerator:
    udev_enumerate_unref(enumerator);
err_enum_free_context:
    if (usbdev != NULL) { udev_device_unref(usbdev); }
    udev_unref(context);
    return result;
}

bool enum_list_devices(struct dev_enum_item ** out, unsigned * out_nb)
{
    struct udev * context;
    struct udev_enumerate * enumerator;
    struct udev_list_entry * dev_first, * dev_current;

    struct dev_enum_item * items = NULL;
    struct dev_enum_item * resized;
    unsigned items_nb = 0;

    bool result = false;

    assert(out != NULL);
    assert(out_nb != NULL);

    if ((context = udev_new()) == NULL) { return false; }
    if ((enumerator = udev_enumerate_new(context)) == NULL) {
        goto err_enum_free_context;
    }
    udev_enumerate_add_match_subsystem(enumerator, "hidraw");

    if (udev_enumerate_scan_devices(enumerator) < 0) {
        goto err_enum_free_enumerator;
    }
    if ((dev_first = udev_enumerate_get_list_entry(enumerator)) == NULL) {
        goto err_enum_free_enumerator;
    }

    udev_list_entry_foreach(dev_current, dev_first) {
        struct udev_device * hiddev, * usbdev;
        Keyleds * device;
        unsigned vendor_id;
        const char * syspath, * devnode, * str;

        /* Get access to device structures */
        syspath = udev_list_entry_get_name(dev_current);
        if ((hiddev = udev_device_new_from_syspath(context, syspath)) == NULL) { continue; }
        usbdev = udev_device_get_parent_with_subsystem_devtype(hiddev, "usb", "usb_device");
        if (usbdev == NULL) { goto err_enum_release_device; }

        /* Get basic device information */
        if ((devnode = udev_device_get_devnode(hiddev)) == NULL) {
            goto err_enum_release_device;
        }

        if ((str = udev_device_get_sysattr_value(usbdev, "idVendor")) == NULL) {
            goto err_enum_release_device;
        }
        vendor_id = strtoul(str, NULL, 16);

        /* Filter out unwanted devices */
        if (vendor_id != LOGITECH_VENDOR_ID) { goto err_enum_release_device; }

        /* Attempt to open device */
        device = keyleds_open(devnode, KEYLEDSCTL_APP_ID);
        if (device == NULL) { goto err_enum_release_device; }
        keyleds_close(device);

        /* Fill info structure */
        resized = realloc(items, (items_nb + 1) * sizeof(items[0]));
        if (resized == NULL) {
            udev_device_unref(hiddev);
            goto err_enum_free_items;
        }
        items = resized;
        if (!fill_info_structure(usbdev, hiddev, &items[items_nb])) {
            udev_device_unref(hiddev);
            goto err_enum_free_items;
        }
        items_nb += 1;

err_enum_release_device:
        udev_device_unref(hiddev);
    }

    /* enum_free_list walks until a NULL path, so the list must carry a terminator */
    resized = realloc(items, (items_nb + 1) * sizeof(items[0]));
    if (resized == NULL) { goto err_enum_free_items; }
    items = resized;
    items[items_nb].path = NULL;

    *out = items;
    *out_nb = items_nb;
    items = NULL;               /* ownership handed to the caller */
    result = true;

err_enum_free_items:
    if (items != NULL) {
        for (unsigned idx = 0; idx < items_nb; idx += 1) {
            free(items[idx].path);
            free(items[idx].serial);
            free(items[idx].description);
        }
        free(items);
    }
err_enum_free_enumerator:
    udev_enumerate_unref(enumerator);
err_enum_free_context:
    udev_unref(context);
    return result;
}
