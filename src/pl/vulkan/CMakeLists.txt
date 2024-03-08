target_sources(libpl
PUBLIC FILE_SET CXX_MODULES FILES
    _module.cppm
    config.cppm
    renderer.cppm
    error.cppm

PRIVATE
    config.cpp
    renderer.cpp
)
