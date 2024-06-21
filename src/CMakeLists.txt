############################################################################
#    Copyright (C) <2019-2023>, winsoft666, <winsoft666@outlook.com>.
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
set (PACKAGE_VERSION 3.1)

if(ZOE_BUILD_SHARED_LIBS)
	set(ZOE_LIB_NAME zoe)
	set(ZOE_STATIC NO)
else()
	set(ZOE_LIB_NAME zoe-static)
	set(ZOE_STATIC YES)
endif()

if (MSVC)
  add_definitions(
    -D_WIN32
  )
endif()


if (MSVC AND ZOE_USE_STATIC_CRT)
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



file(GLOB HEADER_FILES 		../include/zoe/*.h ../include/zoe/*.hpp)
file(GLOB SOURCE_FILES 		./*.cpp ./*.h ./*.hpp)

group_sources(${HEADER_FILES} ${SOURCE_FILES})

if(ZOE_BUILD_SHARED_LIBS AND MSVC)
  file(GLOB_RECURSE RES_FILES "*.rc") 
  source_group("Resources" ${RES_FILES})
endif()

if(ZOE_BUILD_SHARED_LIBS)
	add_definitions(-DZOE_EXPORTS)
	
	add_library(
		${ZOE_LIB_NAME}
		SHARED
		${HEADER_FILES}
		${SOURCE_FILES}
		${RES_FILES}
	)
	
	# set output name prefix/suffix
	set_target_properties(${ZOE_LIB_NAME} PROPERTIES 
		OUTPUT_NAME ${ZOE_LIB_NAME}
		DEBUG_OUTPUT_NAME ${ZOE_LIB_NAME}${DEBUG_LIBNAME_SUFFIX})
else()
	add_definitions(-DZOE_STATIC)
	
	add_library(
		${ZOE_LIB_NAME}
		STATIC
		${HEADER_FILES}
		${SOURCE_FILES}
	)
	
	# set output name prefix/suffix
    set_target_properties(${ZOE_LIB_NAME} PROPERTIES 
		OUTPUT_NAME ${ZOE_LIB_NAME}
		DEBUG_OUTPUT_NAME ${ZOE_LIB_NAME}${DEBUG_LIBNAME_SUFFIX})
		
	set_target_properties(${ZOE_LIB_NAME} PROPERTIES 
		INTERFACE_COMPILE_DEFINITIONS ZOE_STATIC)
endif()


# CURL
find_package(CURL REQUIRED)
target_link_libraries(${ZOE_LIB_NAME} PUBLIC 
	${CURL_LIBRARIES})
target_include_directories(${ZOE_LIB_NAME} PUBLIC 
	${CURL_INCLUDE_DIRS})

# OpenSSL
find_package(OpenSSL)
if(OpenSSL_FOUND)
  add_definitions(-DWITH_OPENSSL)
  target_link_libraries(${ZOE_LIB_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()


if (WIN32 OR _WIN32)
	target_link_libraries(${ZOE_LIB_NAME} PUBLIC 
		Ws2_32.lib Crypt32.lib)
endif()

install(TARGETS ${ZOE_LIB_NAME}
        EXPORT zoe-targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(EXPORT zoe-targets
    NAMESPACE zoe::
    DESTINATION share/zoe
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/zoe-config.cmake.in"
[[include(CMakeFindDependencyMacro)
find_dependency(CURL)
find_dependency(OpenSSL)
include("${CMAKE_CURRENT_LIST_DIR}/zoe-targets.cmake")
]])
configure_file("${CMAKE_CURRENT_BINARY_DIR}/zoe-config.cmake.in" "${CMAKE_CURRENT_BINARY_DIR}/zoe-config.cmake" @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zoe-config.cmake DESTINATION share/zoe)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    zoeConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zoeConfigVersion.cmake
        DESTINATION share/zoe)

install(DIRECTORY ../include/zoe 	DESTINATION include)