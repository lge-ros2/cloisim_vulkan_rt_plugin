find_program(CLOISIM_GLSLC glslc REQUIRED)

set(CLOISIM_SHADER_SOURCES
    depth.rgen
    depth.rmiss
    depth.rchit
    lidar.rgen
    lidar.rmiss
    lidar.rchit
)

set(CLOISIM_SHADER_OUTPUT_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/shaders")

set(CLOISIM_SHADER_OUTPUTS)

foreach(shader IN LISTS CLOISIM_SHADER_SOURCES)
    set(source "${CMAKE_CURRENT_SOURCE_DIR}/shaders/${shader}")
    set(output "${CLOISIM_SHADER_OUTPUT_DIR}/${shader}.spv")

    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory
                "${CLOISIM_SHADER_OUTPUT_DIR}"
        COMMAND "${CLOISIM_GLSLC}"
                --target-env=vulkan1.2
                -O
                -o "${output}"
                "${source}"
        DEPENDS "${source}"
        VERBATIM
    )

    list(APPEND CLOISIM_SHADER_OUTPUTS "${output}")
endforeach()

add_custom_target(
    cloisim_vulkan_rt_shaders ALL
    DEPENDS ${CLOISIM_SHADER_OUTPUTS}
)

add_dependencies(
    cloisim_vulkan_rt
    cloisim_vulkan_rt_shaders
)
