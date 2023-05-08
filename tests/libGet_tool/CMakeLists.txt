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

set (CMAKE_CXX_STANDARD 11)

set(EXE_NAME libGet_tool)

if (MSVC AND USE_STATIC_CRT)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

if (NOT BUILD_SHARED_LIBS)
	add_definitions(-DLIBGET_STATIC)
endif()

file(GLOB SOURCE_FILES 			./*.cpp)

add_executable(
	${EXE_NAME}
	${SOURCE_FILES}
	)

# CURL
find_package(CURL REQUIRED)
target_link_libraries(${EXE_NAME} ${CURL_LIBRARIES})

# OpenSSL
find_package(OpenSSL)
if(OpenSSL_FOUND)
  target_link_libraries(${EXE_NAME} OpenSSL::SSL OpenSSL::Crypto)
endif()

# Win32 Console
if (WIN32 OR _WIN32)
	set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(${EXE_NAME} PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")
endif()


if(BUILD_SHARED_LIBS)
	add_dependencies(${EXE_NAME} libGet)
	
	target_link_libraries(${EXE_NAME} 
		$<TARGET_LINKER_FILE:libGet> )
else()
	add_dependencies(${EXE_NAME} libGet-static)
	
	target_link_libraries(${EXE_NAME} 
		$<TARGET_LINKER_FILE:libGet-static> )

endif()

