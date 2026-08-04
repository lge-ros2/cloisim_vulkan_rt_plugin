#pragma once

#include "rt_pipeline.h"
#include "shader_binding_table.h"

#include <vulkan/vulkan.h>

// Records a lidar ray-trace dispatch (binding 0=accel struct, 1=range
// output storage buffer, 2=params UBO). Mirrors DepthTraceRecorder, with a
// samplesH x samplesV x 1 dispatch instead of a screen-size image.
class LidarTraceRecorder
{
public:
    bool Initialize(
        VkDevice device,
        const RtPipeline* pipeline,
        const ShaderBindingTable* sbt);

    void Shutdown();

    bool Record(
        VkCommandBuffer commandBuffer,
        VkAccelerationStructureKHR tlas,
        VkBuffer outputBuffer,
        VkDeviceSize outputBufferSize,
        VkBuffer paramsUbo,
        VkDeviceSize paramsUboSize,
        uint32_t samplesH,
        uint32_t samplesV);

private:
    bool EnsureDescriptorResources();
    bool UpdateDescriptors(
        VkAccelerationStructureKHR tlas,
        VkBuffer outputBuffer,
        VkDeviceSize outputBufferSize,
        VkBuffer paramsUbo,
        VkDeviceSize paramsUboSize);

    VkDevice device_ = VK_NULL_HANDLE;
    const RtPipeline* pipeline_ = nullptr;
    const ShaderBindingTable* sbt_ = nullptr;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};
