#pragma once

#include <vector>
#include <vulkan/vulkan.h>

// Descriptor set layouts for each of this plugin's ray-tracing pipelines.
// Extracted here so RtPipeline::Create stays generic (it used to hard-code
// the depth pipeline's 2-binding layout) and new pipelines (e.g. lidar) can
// supply their own without touching RtPipeline itself.

inline std::vector<VkDescriptorSetLayoutBinding> DepthDescriptorBindings()
{
    return {
        {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
    };
}

inline std::vector<VkDescriptorSetLayoutBinding> LidarDescriptorBindings()
{
    return {
        {0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
        {2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr},
    };
}
