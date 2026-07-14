#pragma once

#include "rt_pipeline.h"
#include "shader_binding_table.h"

#include <vulkan/vulkan.h>

class DepthTraceRecorder
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
        VkImageView outputImageView,
        uint32_t width,
        uint32_t height);

private:
    bool EnsureDescriptorResources();
    bool UpdateDescriptors(
        VkAccelerationStructureKHR tlas,
        VkImageView outputImageView);

    VkDevice device_ = VK_NULL_HANDLE;
    const RtPipeline* pipeline_ = nullptr;
    const ShaderBindingTable* sbt_ = nullptr;
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
};
