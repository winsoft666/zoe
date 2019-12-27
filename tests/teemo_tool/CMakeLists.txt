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

set(EXE_NAME teemo_tool)

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
	add_definitions(-DTEEMO_STATIC)
endif()

file(GLOB SOURCE_FILES 			./*.cpp ../md5.h ../md5.cpp)

add_executable(
	${EXE_NAME}
	${SOURCE_FILES}
	)

# cpprestsdk
find_package(cpprestsdk REQUIRED)
target_link_libraries(${EXE_NAME} cpprestsdk::cpprest)

# CURL
find_package(CURL REQUIRED)
target_link_libraries(${EXE_NAME} ${CURL_LIBRARIES})

# OpenSSL
find_package(OpenSSL REQUIRED)
target_link_libraries(${EXE_NAME} OpenSSL::SSL OpenSSL::Crypto)

# Win32 Console
if (WIN32 OR _WIN32)
	set_target_properties(${EXE_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(${EXE_NAME} PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")
endif()


if(BUILD_SHARED_LIBS)
	add_dependencies(${EXE_NAME} teemo)
	
	target_link_libraries(${EXE_NAME} 
		$<TARGET_LINKER_FILE:teemo> )
else()
	add_dependencies(${EXE_NAME} teemo-static)
	
	target_link_libraries(${EXE_NAME} 
		$<TARGET_LINKER_FILE:teemo-static> )

endif()

