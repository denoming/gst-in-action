include(GNUInstallDirs)

set(TARGET Basic16)

add_executable(${TARGET} "")
add_executable(Gst::Basic16 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic16
)

target_sources(${TARGET}
    PRIVATE
        Basic16.cpp
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
