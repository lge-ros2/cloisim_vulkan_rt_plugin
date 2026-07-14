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
