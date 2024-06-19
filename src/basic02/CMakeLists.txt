include(GNUInstallDirs)

set(TARGET Basic02)

add_executable(${TARGET} "")
add_executable(Gst::Basic02 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic02
)

target_sources(${TARGET}
    PRIVATE
        basic02.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
            PkgConfig::GStreamerPluginsBase
            PkgConfig::GStreamerPluginsBad
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
