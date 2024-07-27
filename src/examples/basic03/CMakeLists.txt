include(GNUInstallDirs)

set(TARGET Basic03)

add_executable(${TARGET} "")
add_executable(Gst::Basic03 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic03
)

target_sources(${TARGET}
    PRIVATE
        basic03.cpp
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
