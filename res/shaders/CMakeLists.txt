set(SHADER_SOURCES
    shader.vert
    shader.frag
)

set(COMPILED_SHADER_SOURCES)

foreach(src_file ${SHADER_SOURCES})
    set(out_file "${src_file}.spv")
    add_custom_command(
        OUTPUT ${out_file}
        COMMAND glslc ${CMAKE_CURRENT_SOURCE_DIR}/${src_file} -o ${out_file}
        DEPENDS ${src_file}
        VERBATIM)
    list(APPEND COMPILED_SHADER_SOURCES ${out_file})
endforeach()

add_custom_target(glsl_shaders DEPENDS ${COMPILED_SHADER_SOURCES})
add_dependencies("${PROJECT_NAME}" glsl_shaders)
