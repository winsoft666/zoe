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

file(GLOB SOURCE_FILES 	./*.cpp ./*.h ./*.hpp)

if(ZOE_BUILD_SHARED_LIBS AND MSVC)
  file(GLOB_RECURSE RES_FILES "*.rc") 
endif()

if(ZOE_BUILD_SHARED_LIBS)	
	add_library(
		zoe
		SHARED
		../include/zoe/zoe.h
		${SOURCE_FILES}
		${RES_FILES}
	)

	target_compile_definitions(zoe
		PRIVATE ZOE_EXPORTS
	)
else()
	add_library(
		zoe
		STATIC
		../include/zoe/zoe.h
		${SOURCE_FILES}
	)
	
	target_compile_definitions(zoe
		PUBLIC ZOE_STATIC 
	)
endif()

target_compile_definitions(zoe
	PRIVATE UNICODE _UNICODE NOMINMAX
)

# set output name
set_target_properties(zoe PROPERTIES 
	OUTPUT_NAME $<IF:$<BOOL:${ZOE_BUILD_SHARED_LIBS}>,zoe,zoe-static>
	DEBUG_OUTPUT_NAME $<IF:$<BOOL:${ZOE_BUILD_SHARED_LIBS}>,zoe-d,zoe-static-d>)

target_include_directories(zoe
	PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
	PUBLIC $<INSTALL_INTERFACE:include>
)

# CURL
find_package(CURL REQUIRED)
target_link_libraries(zoe PRIVATE CURL::libcurl)

# OpenSSL
find_package(OpenSSL)

if(OpenSSL_FOUND)
	target_link_libraries(zoe PRIVATE OpenSSL::SSL)
	target_compile_definitions(zoe PRIVATE WITH_OPENSSL)
endif()

set(ZOE_REQUIRES "libcurl")
if(OpenSSL_FOUND)
	string(APPEND ZOE_REQUIRES " libssl")
endif()

set(ZOE_LIBS "")
set(ZOE_PRIVATE_LIBS "")
set(ZOE_LIB_NAME "$<TARGET_FILE_BASE_NAME:zoe>")

if (WIN32)
    target_link_libraries(zoe PRIVATE ws2_32 crypt32)
    if(ZOE_BUILD_SHARED_LIBS)
        set(ZOE_PRIVATE_LIBS "${ZOE_PRIVATE_LIBS} -lws2_32 -lcrypt32")
	else()
		set(ZOE_LIBS "${ZOE_LIBS} -lws2_32 -lcrypt32")
    endif()
endif()

set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
target_link_libraries(zoe PRIVATE Threads::Threads)

if(CMAKE_USE_PTHREADS_INIT)
    if(ZOE_BUILD_SHARED_LIBS)
        set(ZOE_PRIVATE_LIBS "${ZOE_PRIVATE_LIBS} -pthread")
	else()
		set(ZOE_LIBS "${ZOE_LIBS} -pthread")
    endif()
endif()

install(TARGETS zoe
		EXPORT zoeTargets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ../include/zoe DESTINATION include)

install(EXPORT zoeTargets
	NAMESPACE zoe::
	DESTINATION share/zoe
)

configure_file(
	"${PROJECT_SOURCE_DIR}/zoe.pc.in"
	"${CMAKE_CURRENT_BINARY_DIR}/zoe.pc.in.configured"
  @ONLY
)

file(GENERATE
	OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/zoe.pc"
    INPUT "${CMAKE_CURRENT_BINARY_DIR}/zoe.pc.in.configured"
)

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/zoe.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)

include(CMakePackageConfigHelpers)

configure_package_config_file(zoeConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/zoeConfig.cmake
	INSTALL_DESTINATION share/zoe
	NO_SET_AND_CHECK_MACRO
	NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
	zoeConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY AnyNewerVersion
)

install(FILES
	${CMAKE_CURRENT_BINARY_DIR}/zoeConfig.cmake
	${CMAKE_CURRENT_BINARY_DIR}/zoeConfigVersion.cmake
	DESTINATION share/zoe
)
