cmake_minimum_required(VERSION 3.22)

project(cloisim_vulkan_rt VERSION 0.1.0 LANGUAGES CXX)

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
