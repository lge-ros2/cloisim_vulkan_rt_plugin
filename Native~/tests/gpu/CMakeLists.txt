add_executable(cloisim_rt_gpu_device_test
    gpu_device_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/gpu_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_dispatch.cpp
)

target_include_directories(cloisim_rt_gpu_device_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(cloisim_rt_gpu_device_test PRIVATE Vulkan::Vulkan)
target_compile_features(cloisim_rt_gpu_device_test PRIVATE cxx_std_20)

target_compile_options(cloisim_rt_gpu_device_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(NAME cloisim_rt_gpu_device_test COMMAND cloisim_rt_gpu_device_test)
set_tests_properties(cloisim_rt_gpu_device_test PROPERTIES
    LABELS "gpu;vulkan;raytracing"
    SKIP_RETURN_CODE 77
    TIMEOUT 30
)

add_executable(cloisim_rt_gpu_execution_test
    rt_execution_test.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/gpu_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_dispatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/acceleration_structure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/rt_scene_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/tlas_instance_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/shader_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/rt_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/shader_binding_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/depth_trace_recorder.cpp
)

target_include_directories(cloisim_rt_gpu_execution_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(cloisim_rt_gpu_execution_test PRIVATE
    Vulkan::Vulkan
)

target_compile_features(
    cloisim_rt_gpu_execution_test
    PRIVATE
    cxx_std_20
)

target_compile_options(cloisim_rt_gpu_execution_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_definitions(cloisim_rt_gpu_execution_test PRIVATE
    CLOISIM_RT_TEST_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/../../shaders"
)

add_dependencies(
    cloisim_rt_gpu_execution_test
    cloisim_vulkan_rt_shaders
)

add_test(
    NAME cloisim_rt_gpu_execution_test
    COMMAND cloisim_rt_gpu_execution_test
)

set_tests_properties(
    cloisim_rt_gpu_execution_test
    PROPERTIES
        LABELS "gpu;vulkan;raytracing;execution"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)

add_executable(cloisim_rt_gpu_lidar_execution_test
    lidar_execution_test.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_memory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/gpu_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/vulkan_dispatch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/acceleration_structure.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/rt_scene_builder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/tlas_instance_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/shader_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/rt_pipeline.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/shader_binding_table.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src/lidar_trace_recorder.cpp
)

target_include_directories(cloisim_rt_gpu_lidar_execution_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
)

target_link_libraries(cloisim_rt_gpu_lidar_execution_test PRIVATE
    Vulkan::Vulkan
)

target_compile_features(
    cloisim_rt_gpu_lidar_execution_test
    PRIVATE
    cxx_std_20
)

target_compile_options(cloisim_rt_gpu_lidar_execution_test PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_compile_definitions(cloisim_rt_gpu_lidar_execution_test PRIVATE
    CLOISIM_RT_TEST_SHADER_DIR="${CMAKE_CURRENT_BINARY_DIR}/../../shaders"
)

add_dependencies(
    cloisim_rt_gpu_lidar_execution_test
    cloisim_vulkan_rt_shaders
)

add_test(
    NAME cloisim_rt_gpu_lidar_execution_test
    COMMAND cloisim_rt_gpu_lidar_execution_test
)

set_tests_properties(
    cloisim_rt_gpu_lidar_execution_test
    PROPERTIES
        LABELS "gpu;vulkan;raytracing;execution;lidar"
        SKIP_RETURN_CODE 77
        TIMEOUT 30
)
