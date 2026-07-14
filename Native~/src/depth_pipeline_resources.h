#pragma once

#include "rt_pipeline.h"
#include "shader_binding_table.h"
#include "vulkan_dispatch.h"

#include <filesystem>
#include <vulkan/vulkan.h>

class DepthPipelineResources
{
public:
    bool Initialize(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch,
        const std::filesystem::path& shaderDirectory);

    void Shutdown();

    bool IsReady() const { return ready_; }
    const RtPipeline& Pipeline() const { return pipeline_; }
    const ShaderBindingTable& Sbt() const { return sbt_; }

private:
    RtPipeline pipeline_;
    ShaderBindingTable sbt_;
    bool ready_ = false;
};
