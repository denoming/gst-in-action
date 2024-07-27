set(TARGET common)

add_library(${TARGET} STATIC)
add_library(Gst::Common ALIAS ${TARGET})

target_include_directories(${TARGET}
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
)

target_link_libraries(${TARGET}
    PUBLIC PkgConfig::GStreamer
)

target_sources(${TARGET}
    PRIVATE src/Utils.cpp
)

target_compile_features(${TARGET} PUBLIC cxx_std_20)
