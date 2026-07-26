/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 * Copyright (C) 2025 Jérôme Poulin, jeromepoulin@gmail.com
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
#include "keyledsd/tools/EvdevWatcher.h"
#include "keyledsd/logging.h"

#include <libevdev/libevdev.h>
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

LOGGING("evdev-watcher");

using keyleds::tools::EvdevWatcher;

static void closeCallback(uv_handle_t * handle)
{
    auto * device = static_cast<EvdevWatcher::Device *>(handle->data);
    if (device->evdev) {
        libevdev_free(device->evdev);
    }
    if (device->fd >= 0) {
        close(device->fd);
    }
    delete device;
}

namespace {
    template <typename List>
    auto findDevice(List & devices, const std::string & devNode)
    {
        return std::find_if(devices.begin(), devices.end(),
                            [&](const auto & d) { return d->devNode == devNode; });
    }
} // namespace

EvdevWatcher::EvdevWatcher(uv_loop_t & loop)
 : m_loop(loop)
{
}

EvdevWatcher::~EvdevWatcher()
{
    for (auto & device : m_devices) {
        uv_poll_stop(&device->poll);
        uv_close(reinterpret_cast<uv_handle_t *>(&device->poll), closeCallback);
        device.release();
    }
}

void EvdevWatcher::addDevice(const std::string & devNode)
{
    if (findDevice(m_devices, devNode) != m_devices.end()) { return; }

    int fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        WARNING("cannot open ", devNode, ": ", std::strerror(errno));
        return;
    }

    struct libevdev * evdev = nullptr;
    int rc = libevdev_new_from_fd(fd, &evdev);
    if (rc < 0) {
        WARNING("cannot create evdev for ", devNode, ": ", std::strerror(-rc));
        close(fd);
        return;
    }

    if (!libevdev_has_event_type(evdev, EV_KEY)) {
        DEBUG(devNode, " is not a keyboard, skipping");
        libevdev_free(evdev);
        close(fd);
        return;
    }

    auto device = std::make_unique<Device>();
    device->watcher = this;
    device->devNode = devNode;
    device->fd = fd;
    device->evdev = evdev;
    device->poll.data = device.get();

    if ((rc = uv_poll_init(&m_loop, &device->poll, fd)) < 0) {
        WARNING("cannot poll ", devNode, ": ", uv_strerror(rc));
        libevdev_free(evdev);
        close(fd);
        return;
    }

    // The loop owns the handle from here on, so teardown must go through uv_close
    if ((rc = uv_poll_start(&device->poll, UV_READABLE, &EvdevWatcher::pollCallback)) < 0) {
        WARNING("cannot watch ", devNode, ": ", uv_strerror(rc));
        auto * orphan = device.release();
        uv_close(reinterpret_cast<uv_handle_t *>(&orphan->poll), closeCallback);
        return;
    }

    NOTICE("watching evdev keyboard ", devNode);
    m_devices.emplace_back(std::move(device));
}

void EvdevWatcher::removeDevice(const std::string & devNode)
{
    auto it = findDevice(m_devices, devNode);
    if (it == m_devices.end()) { return; }

    NOTICE("stopped watching evdev keyboard ", devNode);

    auto * device = it->release();
    m_devices.erase(it);

    uv_poll_stop(&device->poll);
    uv_close(reinterpret_cast<uv_handle_t *>(&device->poll), closeCallback);
}

bool EvdevWatcher::isWatching(const std::string & devNode) const
{
    return findDevice(m_devices, devNode) != m_devices.end();
}

void EvdevWatcher::pollCallback(uv_poll_t * handle, int status, int events)
{
    auto * device = static_cast<Device *>(handle->data);

    // Unplugging is reported as an error rather than a readable event, and the
    // handle is already stopped. Keeping the node would shadow the X11 input
    // path for whatever the kernel gives the same event number to next.
    if (status < 0) {
        WARNING("lost ", device->devNode, ": ", uv_strerror(status));
        device->watcher->removeDevice(device->devNode);
        return;
    }
    if (!(events & UV_READABLE)) { return; }

    device->watcher->onReadable(*device);
}

void EvdevWatcher::onReadable(Device & device)
{
    auto flags = LIBEVDEV_READ_FLAG_NORMAL;

    for (;;) {
        struct input_event ev;
        int rc = libevdev_next_event(device.evdev, flags, &ev);
        if (rc == -EAGAIN) {
            if (flags == LIBEVDEV_READ_FLAG_NORMAL) { break; }
            flags = LIBEVDEV_READ_FLAG_NORMAL;      // resync done, resume reading
            continue;
        }
        if (rc == -ENODEV) {
            // Node is gone: without dropping it the poll would wake us forever
            removeDevice(device.devNode);
            return;                                 // device is queued for deletion
        }
        if (rc < 0) {
            WARNING("error reading from ", device.devNode, ": ", std::strerror(-rc));
            break;
        }
        // Kernel dropped events: the synthesized ones carry the state we missed
        if (rc == LIBEVDEV_READ_STATUS_SYNC) { flags = LIBEVDEV_READ_FLAG_SYNC; }

        if (ev.type == EV_KEY && ev.value != 2) {
            DEBUG("key ", ev.code, " ", ev.value ? "pressed" : "released",
                  " on ", device.devNode);
            keyEventReceived.emit(device.devNode, ev.code, ev.value == 1);
        }
    }
}
