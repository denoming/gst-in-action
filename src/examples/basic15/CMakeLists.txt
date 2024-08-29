include(GNUInstallDirs)

set(TARGET Basic15)

add_executable(${TARGET} "")
add_executable(Gst::Basic15 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic15
)

target_sources(${TARGET}
    PRIVATE
        Basic15.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
    PRIVATE Gst::Common
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
