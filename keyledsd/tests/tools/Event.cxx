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
#include "keyledsd/logging.h"
#include "keyledsd/tools/Event.h"
#include <gtest/gtest.h>
#include <stdexcept>
#include <unistd.h>
#include <uv.h>

LOGGING("test-event");

using keyleds::tools::FDWatcher;

// A subscriber that throws must not unwind into libuv's C frames, where it would
// reach std::terminate instead of any handler.
TEST(FDWatcherTest, exceptionContained)
{
    uv_loop_t loop;
    ASSERT_EQ(0, uv_loop_init(&loop));

    int fds[2];
    ASSERT_EQ(0, pipe(fds));

    bool handlerCalled = false;
    {
        FDWatcher watcher(fds[0], FDWatcher::Read,
            [&](FDWatcher::events) {
                handlerCalled = true;
                throw std::runtime_error("simulated handler failure");
            },
            loop);

        char byte = 'x';
        ASSERT_EQ(1, write(fds[1], &byte, 1));

        uv_run(&loop, UV_RUN_NOWAIT);
    }

    uv_run(&loop, UV_RUN_NOWAIT);   // let the close callback release the handle
    uv_loop_close(&loop);

    close(fds[0]);
    close(fds[1]);

    EXPECT_TRUE(handlerCalled);
}
