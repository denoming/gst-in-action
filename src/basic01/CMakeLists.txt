include(GNUInstallDirs)

set(TARGET Basic01)

add_executable(${TARGET} "")
add_executable(Gst::Basic01 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic01
)

target_sources(${TARGET}
    PRIVATE
        basic01.cpp
)

target_compile_features(${TARGET} PRIVATE cxx_std_17)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
