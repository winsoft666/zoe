###############################################################################
# Copyright (C) 2018 - 2020, winsoft666, <winsoft666@outlook.com>.
#
# THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
# EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
#
# Expect bugs
#
# Please use and enjoy. Please let me know of any bugs/improvements
# that you have found/implemented and I will fix/incorporate them into this
# file.
###############################################################################

if(BUILD_SHARED_LIBS)
	set(LIB_NAME easyfiledownload)
else()
	set(LIB_NAME easyfiledownload-static)
endif()


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



file(GLOB HEADER_FILES 		../include/*.h ../include/*.hpp)
file(GLOB SOURCE_FILES 		./*.cpp)

source_group("Source Files" 			FILES ${SOURCE_FILES})
source_group("Header Files" 			FILES ${HEADER_FILES})

if(BUILD_SHARED_LIBS)
	add_definitions(-DEFD_EXPORTS)
	set(EFD_STATIC NO)
	
	add_library(
		${LIB_NAME}
		SHARED
		${HEADER_FILES}
		${SOURCE_FILES}
	)
	
	# set output name prefix/suffix
	set_target_properties(${LIB_NAME} PROPERTIES 
		OUTPUT_NAME ${LIB_NAME}
		DEBUG_OUTPUT_NAME ${LIB_NAME}${DEBUG_LIBNAME_SUFFIX})
else()
	add_definitions(-DEFD_STATIC)
	set(EFD_STATIC YES)
	
	add_library(
		${LIB_NAME}
		STATIC
		${HEADER_FILES}
		${SOURCE_FILES}
	)
	
	# set output name prefix/suffix
    set_target_properties(${LIB_NAME} PROPERTIES 
		OUTPUT_NAME ${LIB_NAME}
		DEBUG_OUTPUT_NAME ${LIB_NAME}${DEBUG_LIBNAME_SUFFIX})
		
	set_target_properties(${LIB_NAME} PROPERTIES 
		INTERFACE_COMPILE_DEFINITIONS EFD_STATIC)
endif()


# CURL
find_package(CURL REQUIRED)
target_link_libraries(${LIB_NAME} PUBLIC 
	${CURL_LIBRARIES})
target_include_directories(${LIB_NAME} PUBLIC 
	${CURL_INCLUDE_DIRS})

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(${LIB_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)

# cpprestsdk
find_package(cpprestsdk REQUIRED)
target_link_libraries(${LIB_NAME} PUBLIC cpprestsdk::cpprest)


if (WIN32 OR _WIN32)
	target_link_libraries(${LIB_NAME} PUBLIC 
		Ws2_32.lib Crypt32.lib)
endif()

# Set output directory
set_target_properties(${LIB_NAME}
	PROPERTIES
	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

target_include_directories(${LIB_NAME} INTERFACE
	$<INSTALL_INTERFACE:include>)

install(TARGETS ${LIB_NAME}
	EXPORT ${LIB_NAME}-target
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib)


install(FILES ${HEADER_FILES} 			DESTINATION include)


install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../CMake/easyfiledownload-config.cmake 
	DESTINATION ${EFD_INSTALL_CMAKE_DIR})

export(TARGETS ${LIB_NAME}
       APPEND FILE ${PROJECT_BINARY_DIR}/${LIB_NAME}-target.cmake
)

install(EXPORT ${LIB_NAME}-target
        FILE ${LIB_NAME}-target.cmake
        DESTINATION ${EFD_INSTALL_CMAKE_DIR}
)
