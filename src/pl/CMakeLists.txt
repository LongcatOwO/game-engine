add_library(libpl)
target_sources(libpl PUBLIC FILE_SET CXX_MODULES)

add_subdirectory(core)
add_subdirectory(vulkan)
