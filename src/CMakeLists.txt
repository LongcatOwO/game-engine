target_sources("${PROJECT_NAME}"
PRIVATE FILE_SET CXX_MODULES FILES
    main.cppm

PRIVATE
    main.cpp
)

add_subdirectory(pl)

target_link_libraries("${PROJECT_NAME}" libpl)
