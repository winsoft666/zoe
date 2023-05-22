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

project(zoe CXX)

function(group_sources)
  foreach(_source IN ITEMS ${ARGN})
  if (IS_ABSOLUTE "${_source}")
      file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
  else()
      set(_source_rel "${_source}")
  endif()
  get_filename_component(_source_path "${_source_rel}" PATH)
  string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
  source_group("${_source_path_msvc}" FILES "${_source}")
  endforeach()
endfunction(group_sources)


option(ZOE_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(ZOE_BUILD_TESTS "Build tests project" ON)
option(ZOE_USE_STATIC_CRT "Set to ON to build with static CRT on Windows (/MT)." OFF)

# Debug Output
message(STATUS "ZOE_BUILD_SHARED_LIBS=${ZOE_BUILD_SHARED_LIBS}")
message(STATUS "ZOE_USE_STATIC_CRT=${ZOE_USE_STATIC_CRT}")
message(STATUS "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}")
message(STATUS "VCPKG_TARGET_TRIPLET=${VCPKG_TARGET_TRIPLET}")
message(STATUS "CMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}")


set(DEBUG_LIBNAME_SUFFIX "-d" 
	CACHE STRING "Optional suffix to append to the library name for a debug build")
mark_as_advanced(DEBUG_LIBNAME_SUFFIX)

# Unicode Support
add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

include_directories(./include)

add_subdirectory(src)

if(ZOE_BUILD_TESTS)
	add_subdirectory(tests)
endif()