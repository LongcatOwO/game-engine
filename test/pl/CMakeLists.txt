add_executable(libpl_test)

target_link_libraries(libpl_test PRIVATE libpl)

target_sources(libpl_test
PRIVATE FILE_SET CXX_MODULES
PRIVATE
    main.cpp
)

add_subdirectory(core)
