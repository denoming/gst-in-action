include(GNUInstallDirs)

set(TARGET Basic07)

add_executable(${TARGET} "")
add_executable(Gst::Basic07 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic07
)

target_sources(${TARGET}
    PRIVATE
        basic07.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
            PkgConfig::GStreamerAudio
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
