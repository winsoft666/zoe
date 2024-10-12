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

add_executable(zoe_tool
	zoe_tool.cpp
)

# Win32 Console
if (WIN32 OR _WIN32)
	set_target_properties(zoe_tool PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(zoe_tool PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")
endif()

# set output name
set_target_properties(zoe_tool PROPERTIES 
	OUTPUT_NAME ZoeTool
	DEBUG_OUTPUT_NAME ZoeTool-d)

add_dependencies(zoe_tool zoe)

target_include_directories(zoe_tool PRIVATE "${CMAKE_SOURCE_DIR}/include")

target_link_libraries(zoe_tool PRIVATE zoe)
