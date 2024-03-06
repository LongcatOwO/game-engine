target_sources(libpl_test
PRIVATE FILE_SET CXX_MODULES FILES
    _module.cppm
    side_effects.cppm
)

add_subdirectory(static_assertion)
