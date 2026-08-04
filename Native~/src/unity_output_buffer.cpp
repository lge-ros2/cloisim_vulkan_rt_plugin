#include "unity_output_buffer.h"

bool UnityOutputBuffer::AccessForStorage(
    IUnityGraphicsVulkanV2* vulkan,
    void* nativeBuffer)
{
    Reset();

    if (vulkan == nullptr || nativeBuffer == nullptr)
        return false;

    return vulkan->AccessBuffer(
        nativeBuffer,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_SHADER_WRITE_BIT,
        kUnityVulkanResourceAccess_PipelineBarrier,
        &buffer_);
}

void UnityOutputBuffer::Reset()
{
    buffer_ = UnityVulkanBuffer{};
}
