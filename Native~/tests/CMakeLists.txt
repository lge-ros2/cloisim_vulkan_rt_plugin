add_executable(cloisim_rt_abi_layout_test
    abi_layout_test.cpp
)
target_include_directories(cloisim_rt_abi_layout_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)
target_compile_features(cloisim_rt_abi_layout_test PRIVATE cxx_std_20)
add_test(NAME cloisim_rt_abi_layout_test COMMAND cloisim_rt_abi_layout_test)

add_executable(cloisim_rt_shader_loader_test
    shader_loader_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/shader_loader.cpp
)
target_include_directories(cloisim_rt_shader_loader_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)
target_compile_features(cloisim_rt_shader_loader_test PRIVATE cxx_std_20)
add_test(NAME cloisim_rt_shader_loader_test COMMAND cloisim_rt_shader_loader_test)

add_executable(cloisim_rt_instance_layout_test
    instance_layout_test.cpp
)
target_link_libraries(cloisim_rt_instance_layout_test PRIVATE Vulkan::Vulkan)
target_compile_features(cloisim_rt_instance_layout_test PRIVATE cxx_std_20)
add_test(NAME cloisim_rt_instance_layout_test COMMAND cloisim_rt_instance_layout_test)

add_test(
    NAME cloisim_rt_spirv_outputs_test
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_DIR=${CMAKE_CURRENT_BINARY_DIR}/../shaders
        -P ${CMAKE_CURRENT_SOURCE_DIR}/verify_spirv.cmake
)
set_tests_properties(
    cloisim_rt_spirv_outputs_test
    PROPERTIES DEPENDS cloisim_rt_shader_loader_test
)
