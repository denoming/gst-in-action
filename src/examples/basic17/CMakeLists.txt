include(GNUInstallDirs)

set(TARGET Basic17)

add_executable(${TARGET} "")
add_executable(Gst::Basic17 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic17
)

target_sources(${TARGET}
    PRIVATE
        Basic17.cpp
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
