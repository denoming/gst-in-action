include(GNUInstallDirs)

set(TARGET Basic14)

add_executable(${TARGET} "")
add_executable(Gst::Basic14 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic14
)

target_sources(${TARGET}
    PRIVATE
        Basic14.cpp
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
