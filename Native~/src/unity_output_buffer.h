#pragma once

#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"

#include <vulkan/vulkan.h>

// Mirrors UnityOutputTexture, but for a Unity GraphicsBuffer/ComputeBuffer's
// native VkBuffer handle (via IUnityGraphicsVulkanV2::AccessBuffer) instead
// of AccessTexture — used to write the lidar trace's range output directly
// into a buffer the C# side reads back with AsyncGPUReadback.
class UnityOutputBuffer
{
public:
    bool AccessForStorage(
        IUnityGraphicsVulkanV2* vulkan,
        void* nativeBuffer);

    void Reset();

    VkBuffer Handle() const { return buffer_.buffer; }
    VkDeviceSize Size() const { return buffer_.sizeInBytes; }
    bool IsValid() const { return buffer_.buffer != VK_NULL_HANDLE; }

private:
    UnityVulkanBuffer buffer_{};
};
