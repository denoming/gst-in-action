include(GNUInstallDirs)

set(TARGET Basic18)

add_executable(${TARGET} "")
add_executable(Gst::Basic18 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic18
)

target_sources(${TARGET}
    PRIVATE
        Basic18.cpp
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
