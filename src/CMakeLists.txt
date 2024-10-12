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
	OUTPUT_NAME libZoe
	DEBUG_OUTPUT_NAME libZoe-d)

target_include_directories(zoe
	PUBLIC $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
	PUBLIC $<INSTALL_INTERFACE:../include>
)

# CURL
find_package(CURL REQUIRED)
if(ZOE_BUILD_SHARED_LIBS)	
	target_link_libraries(zoe PRIVATE ${CURL_LIBRARIES})
else()
	target_link_libraries(zoe PUBLIC ${CURL_LIBRARIES})
endif()

target_include_directories(zoe PRIVATE ${CURL_INCLUDE_DIRS})

# OpenSSL
find_package(OpenSSL)
if(OpenSSL_FOUND)
	target_compile_definitions(zoe PRIVATE WITH_OPENSSL)

	if(ZOE_BUILD_SHARED_LIBS)	
		target_link_libraries(zoe PRIVATE OpenSSL::SSL OpenSSL::Crypto
		)
	else()
		target_link_libraries(zoe PUBLIC OpenSSL::SSL OpenSSL::Crypto
		)
	endif()
endif()


if (WIN32 OR _WIN32)
	if(ZOE_BUILD_SHARED_LIBS)
		target_link_libraries(zoe PRIVATE Ws2_32.lib Crypt32.lib)
	else()
		target_link_libraries(zoe PUBLIC Ws2_32.lib Crypt32.lib)
	endif()
endif()

install(TARGETS zoe
        EXPORT zoeConfig
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        OBJECTS DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY ../include/zoe DESTINATION include)

install(EXPORT zoeConfig
    NAMESPACE zoe::
    DESTINATION share/zoe
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    zoeConfigVersion.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/zoeConfigVersion.cmake DESTINATION share/zoe)
