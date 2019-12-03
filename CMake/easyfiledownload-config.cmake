# easyfiledownload-config.cmake - package configuration file

get_filename_component(SELF_DIR "${CMAKE_CURRENT_LIST_FILE}" PATH)

if(EXISTS ${SELF_DIR}/easyfiledownload-target.cmake)
	include(${SELF_DIR}/easyfiledownload-target.cmake)
endif()

if(EXISTS ${SELF_DIR}/easyfiledownload-static-target.cmake)
	include(${SELF_DIR}/easyfiledownload-static-target.cmake)
endif()
