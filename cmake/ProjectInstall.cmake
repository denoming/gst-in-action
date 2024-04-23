
if (NOT WIN32)
    # Set default install prefix path
    if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/opt/denoming/${PROJECT_NAME}" CACHE PATH "..." FORCE)
    endif()
endif()