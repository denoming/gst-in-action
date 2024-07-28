include(GNUInstallDirs)

set(TARGET Basic00)

add_executable(${TARGET} "")
add_executable(Gst::Basic00 ALIAS ${TARGET})

set_target_properties(${TARGET}
    PROPERTIES
    OUTPUT_NAME basic00
)

target_sources(${TARGET}
    PRIVATE
        basic00.cpp
)

target_link_libraries(${TARGET}
    PRIVATE Gst::Common
)

target_compile_features(${TARGET} PRIVATE cxx_std_20)

install(
    TARGETS ${TARGET}
    COMPONENT MyApp_Runtime
)
