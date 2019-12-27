# teemo-config.cmake - package configuration file

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(EXISTS ${SELF_DIR}/teemo-target.cmake)
	include(${SELF_DIR}/teemo-target.cmake)
endif()

if(EXISTS ${SELF_DIR}/teemo-static-target.cmake)
	include(${SELF_DIR}/teemo-static-target.cmake)
endif()
