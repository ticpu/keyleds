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
#ifndef TOOLS_EVDEVWATCHER_H
#define TOOLS_EVDEVWATCHER_H

#include "keyledsd/tools/Event.h"
#include <string>
#include <vector>
#include <memory>
#include <uv.h>

struct libevdev;

namespace keyleds::tools {

class EvdevWatcher final
{
public:
    struct Device {
        EvdevWatcher *      watcher = nullptr;
        std::string         devNode;
        int                 fd = -1;
        struct libevdev *   evdev = nullptr;
        uv_poll_t           poll;
    };

                    EvdevWatcher(uv_loop_t & loop);
                    ~EvdevWatcher();

    void            addDevice(const std::string & devNode);
    void            removeDevice(const std::string & devNode);

    Callback<const std::string &, int, bool> keyEventReceived;

private:
    void            onReadable(Device & device);
    static void     pollCallback(uv_poll_t * handle, int status, int events);

    uv_loop_t &                         m_loop;
    std::vector<std::unique_ptr<Device>> m_devices;
};

} // namespace keyleds::tools

#endif
