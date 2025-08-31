cmake_minimum_required(VERSION 3.18)

project(gst-in-action
    VERSION 0.1.0
    DESCRIPTION "Gstreamer samples"
)

include(cmake/ProjectOptions.cmake)
include(cmake/ProjectConfigs.cmake)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(cmake/ProjectInstall.cmake)
endif()

add_subdirectory(src)
