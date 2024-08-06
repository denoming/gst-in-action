include(GNUInstallDirs)

set(TARGET Basic05)

add_executable(${TARGET} "")
add_executable(Gst::Basic05 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic05
)

target_sources(${TARGET}
    PRIVATE
        basic05.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
            PkgConfig::GStreamerPluginsBase
            PkgConfig::GStreamerPluginsBad
    PRIVATE Gst::Common
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
