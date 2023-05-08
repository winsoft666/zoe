############################################################################
#    Copyright (C) <2019-2022>, winsoft666, <winsoft666@outlook.com>.
#
#    This program is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http:#www.gnu.org/licenses/>.
############################################################################

cmake_minimum_required(VERSION 3.4 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH};")

project(libGet CXX)

option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_TESTS "Build tests project" ON)
option(USE_STATIC_CRT "Set to ON to build with static CRT on Windows (/MT)." OFF)

if(MSVC)
  set(LIBGET_INSTALL_CMAKE_DIR share/libGet)
elseif(WIN32 AND NOT CYGWIN)
  set(LIBGET_INSTALL_CMAKE_DIR CMake)
else()
  set(LIBGET_INSTALL_CMAKE_DIR lib/cmake/libGet)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Debug Output
message(STATUS "BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}")
message(STATUS "USE_STATIC_CRT=${USE_STATIC_CRT}")
message(STATUS "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
message(STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")


set(DEBUG_LIBNAME_SUFFIX "-d" 
	CACHE STRING "Optional suffix to append to the library name for a debug build")
mark_as_advanced(DEBUG_LIBNAME_SUFFIX)

set(LIBGET_NAMESPACE "libGet" 
	CACHE STRING "Custom library's namespace")
mark_as_advanced(LIBGET_NAMESPACE)

set(LIBGET "Teemo" 
	CACHE STRING "Custom library's export class name")
mark_as_advanced(LIBGET)

# Unicode Support
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/libGet/config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/include/libGet/config.h @ONLY)

include_directories(./include)

add_subdirectory(src)

if(BUILD_TESTS)
	add_subdirectory(tests)
endif()