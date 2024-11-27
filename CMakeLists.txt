############################################################################
#    Copyright (C) <2019-2024>, winsoft666, <winsoft666@outlook.com>.
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

cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake;${CMAKE_MODULE_PATH};")

project(zoe
  VERSION 3.6
  LANGUAGES CXX
)

option(ZOE_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ZOE_BUILD_TESTS "Build tests project" ON)
option(ZOE_USE_STATIC_CRT "Set to ON to build with static CRT on Windows (/MT)." OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (MSVC AND ZOE_USE_STATIC_CRT)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set (CMAKE_CXX_STANDARD 11)

add_subdirectory(src)

if(ZOE_BUILD_TESTS)
	add_subdirectory(tests)
endif()