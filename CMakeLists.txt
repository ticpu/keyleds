# Keyleds -- Gaming keyboard tool
# Copyright (C) 2017-2020 Julien Hartmann, juli1.hartmann@gmail.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required (VERSION 3.10)
project (keyleds VERSION 1.1.1 LANGUAGES C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMakeModules" ${CMAKE_MODULE_PATH})

##############################################################################
# Settings

include(GNUInstallDirs)

option(WITH_KEYLEDSD "build keyledsd daemon" ON)
option(WITH_PYTHON "build python bindings" OFF)
option(WITH_TESTS "build test suite" OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

add_compile_options(
    -Wall -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-Wcast-align>
    $<$<COMPILE_LANGUAGE:CXX>:-Wcast-qual>
    $<$<COMPILE_LANGUAGE:CXX>:-Wconversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wold-style-cast>
    $<$<COMPILE_LANGUAGE:CXX>:-Wsign-conversion>
    $<$<COMPILE_LANGUAGE:CXX>:-Wshadow>
    $<$<COMPILE_LANGUAGE:CXX>:-Wstrict-overflow=3>
)

##############################################################################
# Global dependencies

IF(WITH_TESTS)
    find_package(GTest REQUIRED)
    enable_testing()
ENDIF(WITH_TESTS)

##############################################################################
# Include subprojects

add_subdirectory(libkeyleds)
add_subdirectory(keyledsctl)
IF (WITH_KEYLEDSD)
    add_subdirectory(keyledsd)
ENDIF()
IF (WITH_PYTHON)
    add_subdirectory(python)
ENDIF()

##############################################################################
# Data files

find_package(PkgConfig REQUIRED)
pkg_get_variable(UDEV_RULES_DIR udev udev_dir)
IF(UDEV_RULES_DIR)
    set(UDEV_RULES_DIR ${UDEV_RULES_DIR}/rules.d)
ELSE()
    set(UDEV_RULES_DIR ${CMAKE_INSTALL_PREFIX}/lib/udev/rules.d)
ENDIF()
install(FILES logitech.rules DESTINATION ${UDEV_RULES_DIR}
        RENAME 70-logitech-hidpp.rules)

##############################################################################
# Packaging

# Builds and install-tests the Debian package in a container. Needs only podman,
# so it does not depend on anything this build itself configured.
add_custom_target(deb
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/build-deb.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Debian package in a container"
    USES_TERMINAL)

add_custom_target(rpm
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/packaging/build-rpm.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building RPM package in a container"
    USES_TERMINAL)
