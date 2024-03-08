cmake_minimum_required(VERSION 3.28)

project(vulkan-engine CXX)

# export compile_commands.json for clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Turning off extensions avoids an issue with the clang 16 compiler
# clang 17 and greater can avoid this setting
set(CMAKE_CXX_EXTENSIONS OFF)


########## Compilation flags ##########

# Remove default CMake C++ flags
unset(CMAKE_CXX_FLAGS)
unset(CMAKE_CXX_FLAGS_DEBUG)
unset(CMAKE_CXX_FLAGS_RELWITHDEBINFO)
unset(CMAKE_CXX_FLAGS_MINSIZEREL)
unset(CMAKE_CXX_FLAGS_RELEASE)

# General compilation flags
set(CXX_COMPILE_FLAGS
    -stdlib=libc++

    # Warnings
    -Wall
    -Wextra
    -Wfloat-equal
    -Wundef
    -Wshadow
    -Wpointer-arith
    -Wcast-align
    -Wwrite-strings
    -Wcast-qual
    -Wswitch-default
    -Wswitch-enum
    -Wdouble-promotion
    -Wconversion
    -Wunreachable-code
    -pedantic                           # Pedantic C++

    
    -Werror=return-type                 # Promote missing return warning to error
    -Wno-gnu-statement-expression       # Disable statement expression warning
    
    # Flags
    -fvisibility=hidden                 # Do not export symbols
    -fno-rtti                           # Disable runtime type information (reduce binary size)
    -fno-exceptions                     # Disable exceptions (allow explicit error handling through error code only)
)

# General link flags
set(CXX_LINK_FLAGS
    -stdlib=libc++
)


# Debug compilation flags
set(CXX_COMPILE_FLAGS_DEBUG
    # Sanitizers
    -fsanitize=address                  # Detect memory leak
    -fsanitize=undefined                # Detect undefined behaviour (arithmetic overflow, etc.)
    # -fsanitize=thread                 # Cannot be active at the same time as -fsanitize=address

    # Debug flags
    -Og                                 # Optmize without losing debug information
    -g                                  # Include debug information
)

# Debug link flags
set(CXX_LINK_FLAGS_DEBUG
    # Sanitizers
    -fsanitize=address                  # Detect memory leak
    -fsanitize=undefined                # Detect undefined behaviour (arithmetic overflow, etc.)
)


# RelWithDebInfo compilation flags
set(CXX_COMPILE_FLAGS_RELWITHDEBINFO    
    -O2                                 # Optmization level 2
    -g                                  # Include debug information
    -DNDEBUG                            # Turns off debug specific functionalities in the code
)

# RelWithDebInfo link flags
set(CXX_LINK_FLAGS_RELWITHDEBINFO "")


# MinSizeRel compilation flags
set(CXX_COMPILE_FLAGS_MINSIZEREL
    -Os                                 # Minimum size optimization
    -DNDEBUG                            # Turns off debug specific functionalities in the code
)

# MinSizeRel link flags
set(CXX_LINK_FLAGS_MINSIZEREL "")


# Release compilation flags
set(CXX_COMPILE_FLAGS_RELEASE
    -O3                                 # Optimization level 3 (Maximum)
    -DNDEBUG                            # Turns off debug specific functionalities in the code
    -flto                               # Enable Link-Time Optimization
)

# Release link flags
set(CXX_LINK_FLAGS_RELEASE
    -flto                               # Enable Link-Time Optimization
)

# Add compilation flags
add_compile_options("${CXX_COMPILE_FLAGS}")
add_compile_options("$<$<CONFIG:Debug>:${CXX_COMPILE_FLAGS_DEBUG}>")
add_compile_options("$<$<CONFIG:RelWithDebInfo>:${CXX_COMPILE_FLAGS_RELWITHDEBINFO}>")
add_compile_options("$<$<CONFIG:MinSizeRel>:${CXX_COMPILE_FLAGS_MINSIZEREL}>")
add_compile_options("$<$<CONFIG:Release>:${CXX_COMPILE_FLAGS_RELEASE}>")

# Add link flags
add_link_options("${CXX_LINK_FLAGS}")
add_link_options("$<$<CONFIG:Debug>:${CXX_LINK_FLAGS_DEBUG}>")
add_link_options("$<$<CONFIG:RelWithDebInfo>:${CXX_LINK_FLAGS_RELWITHDEBINFO}>")
add_link_options("$<$<CONFIG:MinSizeRel>:${CXX_LINK_FLAGS_MINSIZEREL}>")
add_link_options("$<$<CONFIG:Release>:${CXX_LINK_FLAGS_RELEASE}>")

# Add compile definitions
add_compile_options(-include "${CMAKE_CURRENT_SOURCE_DIR}/src/lib_config.hpp")


########## External Libraries ##########

# Find Vulkan
find_package(Vulkan REQUIRED)

# Require Vulkan version >= 1.3.256 (earliest version when the Vulkan module was available)
if (${Vulkan_VERSION} VERSION_LESS "1.3.256")
    message(FATAL_ERROR
    "Minimum required Vulkan version for C++ modules is 1.3.256."
    "Found ${Vulkan_VERSION}.")
endif()

# Sets up Vulkan C++ module.
add_library(VulkanHppModule)
target_sources(VulkanHppModule PUBLIC
    FILE_SET CXX_MODULES
    BASE_DIRS ${Vulkan_INCLUDE_DIR}
    FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
)

target_compile_features(VulkanHppModule PUBLIC cxx_std_23)
target_link_libraries(VulkanHppModule PUBLIC Vulkan::Vulkan)

find_package(glfw3 3.3 REQUIRED)
find_package(glm REQUIRED)

link_libraries(
    glfw
    glm::glm
    VulkanHppModule
)

set(PL_RESOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/res")
configure_file("src/project_resource.hpp.in" "src/project_resource.hpp")

include_directories("${CMAKE_CURRENT_BINARY_DIR}/src")

include_directories(src)

add_executable("${PROJECT_NAME}")
add_subdirectory(src)

include_directories(test)

add_subdirectory(test)

add_subdirectory(res)
