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
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
        [&](const auto & d) { return d->devNode == devNode; });
    if (it != m_devices.end()) { return; }

    int fd = open(devNode.c_str(), O_RDONLY | O_NONBLOCK);
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

    uv_poll_init(&m_loop, &device->poll, fd);
    uv_poll_start(&device->poll, UV_READABLE, &EvdevWatcher::pollCallback);

    INFO("watching evdev keyboard ", devNode);
    m_devices.emplace_back(std::move(device));
}

void EvdevWatcher::removeDevice(const std::string & devNode)
{
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
        [&](const auto & d) { return d->devNode == devNode; });
    if (it == m_devices.end()) { return; }

    INFO("stopped watching evdev keyboard ", devNode);

    auto * device = it->release();
    m_devices.erase(it);

    uv_poll_stop(&device->poll);
    uv_close(reinterpret_cast<uv_handle_t *>(&device->poll), closeCallback);
}

void EvdevWatcher::pollCallback(uv_poll_t * handle, int status, int events)
{
    if (status < 0) { return; }
    if (!(events & UV_READABLE)) { return; }

    auto * device = static_cast<Device *>(handle->data);
    device->watcher->onReadable(*device);
}

void EvdevWatcher::onReadable(Device & device)
{
    for (;;) {
        struct input_event ev;
        int rc = libevdev_next_event(device.evdev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
        if (rc == -EAGAIN) {
            break;
        }
        if (rc < 0) {
            WARNING("error reading from ", device.devNode, ": ", std::strerror(-rc));
            break;
        }
        if (rc == LIBEVDEV_READ_STATUS_SYNC) {
            continue;
        }

        if (ev.type == EV_KEY && ev.value != 2) {
            DEBUG("key ", ev.code, " ", ev.value ? "pressed" : "released",
                  " on ", device.devNode);
            keyEventReceived.emit(device.devNode, ev.code, ev.value == 1);
        }
    }
}
