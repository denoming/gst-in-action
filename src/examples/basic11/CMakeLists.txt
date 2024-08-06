include(GNUInstallDirs)

set(TARGET Basic11)

add_executable(${TARGET} "")
add_executable(Gst::Basic11 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic11
)

target_sources(${TARGET}
    PRIVATE
        basic11.cpp
)

target_link_libraries(${TARGET}
    PRIVATE PkgConfig::GStreamer
            PkgConfig::GStreamerBase
            PkgConfig::GStreamerPbUtils
    PRIVATE Gst::Common
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
