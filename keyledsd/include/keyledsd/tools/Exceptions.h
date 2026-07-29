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
#ifndef TOOLS_EXCEPTIONS_H_3A7F2B91
#define TOOLS_EXCEPTIONS_H_3A7F2B91

#include <stdexcept>

namespace keyleds::tools {

/// Environmental fault — exit(1), no core; the daemon cannot continue in place.
class FatalError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

/// Code invariant violation — abort(), core dump is worth keeping.
class InvariantError : public FatalError
{
public:
    using FatalError::FatalError;
};

} // namespace keyleds::tools

#endif
