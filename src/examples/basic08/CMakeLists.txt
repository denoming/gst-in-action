include(GNUInstallDirs)

set(TARGET Basic08)

add_executable(${TARGET} "")
add_executable(Gst::Basic08 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic08
)

target_sources(${TARGET}
    PRIVATE
        basic08.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
            PkgConfig::GStreamerPbUtils
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
