###############################################################################
# Copyright (C) 2018 - 2024, winsoft666, <winsoft666@outlook.com>.
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

file(GLOB SOURCE_FILES 			./*.cpp ./*h ../../src/file_util.cpp ../../src/string_encode.cpp)

add_executable(
	unit_test
	${SOURCE_FILES}
)

add_dependencies(unit_test zoe)

target_include_directories(unit_test 
    PRIVATE ../../src
    PRIVATE ../../include
)
	
# Win32 Console
if (WIN32 OR _WIN32)
	set_target_properties(unit_test PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
	set_target_properties(unit_test PROPERTIES COMPILE_DEFINITIONS "_CONSOLE")
endif()

# set output name
set_target_properties(unit_test PROPERTIES 
	OUTPUT_NAME UnitTest
	DEBUG_OUTPUT_NAME UnitTest-d)

target_link_libraries(unit_test PRIVATE zoe)
