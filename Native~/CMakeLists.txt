cmake_minimum_required(VERSION 3.22)

project(cloisim_vulkan_rt VERSION 0.1.0 LANGUAGES CXX)

option(
    CLOISIM_RT_BUILD_GPU_TESTS
    "Build standalone Vulkan GPU tests"
    OFF
)

option(
    CLOISIM_RT_DEPLOY_RUNTIME
    "Deploy native plugin and SPIR-V shaders to Runtime"
    OFF
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Vulkan REQUIRED)

add_library(cloisim_vulkan_rt SHARED
    src/plugin_entry.cpp
    src/unity_vulkan_bridge.cpp
    src/vulkan_interceptor.cpp
    src/vulkan_dispatch.cpp
    src/vulkan_memory.cpp
    src/gpu_buffer.cpp
    src/acceleration_structure.cpp
    src/rt_scene_builder.cpp
    src/deferred_release_queue.cpp
    src/tlas_instance_buffer.cpp
    src/rt_runtime_context.cpp
    src/rt_pipeline.cpp
    src/shader_binding_table.cpp
    src/shader_loader.cpp
    src/depth_pipeline_resources.cpp
    src/unity_output_texture.cpp
    src/depth_trace_recorder.cpp
    src/blas_cache.cpp
    src/native_scene.cpp
    src/lidar_pipeline_resources.cpp
    src/unity_output_buffer.cpp
    src/lidar_trace_recorder.cpp
)

target_include_directories(cloisim_vulkan_rt PRIVATE
    include
    src
    third_party/unity
    ${Vulkan_INCLUDE_DIRS}
)

foreach(header IUnityInterface.h IUnityGraphics.h IUnityGraphicsVulkan.h)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/${header}")
        message(FATAL_ERROR "Missing Unity PluginAPI header: Native~/third_party/unity/${header}")
    endif()
endforeach()

target_link_libraries(cloisim_vulkan_rt PRIVATE Vulkan::Vulkan dl pthread)

target_compile_options(cloisim_vulkan_rt PRIVATE
    -Wall
    -Wextra
    -Wpedantic
    -Werror=return-type
)

set_target_properties(cloisim_vulkan_rt PROPERTIES
    OUTPUT_NAME "cloisim_vulkan_rt"
)

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CompileShaders.cmake)

if(CLOISIM_RT_DEPLOY_RUNTIME)
    set(
        CLOISIM_RT_RUNTIME_PLUGIN_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../Runtime/Plugins/Linux/x86_64"
    )

    set(
        CLOISIM_RT_RUNTIME_SHADER_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/../Runtime/Shaders"
    )

    add_custom_target(cloisim_vulkan_rt_deploy ALL
        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${CLOISIM_RT_RUNTIME_PLUGIN_DIR}"

        COMMAND
            ${CMAKE_COMMAND} -E make_directory
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:cloisim_vulkan_rt>"
            "${CLOISIM_RT_RUNTIME_PLUGIN_DIR}/$<TARGET_FILE_NAME:cloisim_vulkan_rt>"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/depth.rgen.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/depth.rgen.spv"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/depth.rmiss.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/depth.rmiss.spv"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/depth.rchit.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/depth.rchit.spv"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/lidar.rgen.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/lidar.rgen.spv"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/lidar.rmiss.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/lidar.rmiss.spv"

        COMMAND
            ${CMAKE_COMMAND} -E copy_if_different
            "${CLOISIM_SHADER_OUTPUT_DIR}/lidar.rchit.spv"
            "${CLOISIM_RT_RUNTIME_SHADER_DIR}/lidar.rchit.spv"

        DEPENDS
            cloisim_vulkan_rt
            cloisim_vulkan_rt_shaders

        COMMENT
            "Deploying CLOiSim Vulkan RT runtime artifacts"

        VERBATIM
    )
endif()


include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

if(BUILD_TESTING AND CLOISIM_RT_BUILD_GPU_TESTS)
    add_subdirectory(tests/gpu)
endif()
