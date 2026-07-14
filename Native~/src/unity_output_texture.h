#pragma once

#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"

#include <vulkan/vulkan.h>

class UnityOutputTexture
{
public:
    bool AccessForStorage(
        IUnityGraphicsVulkanV2* vulkan,
        void* nativeTexture);

    void Reset();

    const UnityVulkanImage& Image() const { return image_; }
    VkImageView View() const { return view_; }
    bool IsValid() const
    {
        return image_.image != VK_NULL_HANDLE &&
            view_ != VK_NULL_HANDLE;
    }

    bool EnsureView(VkDevice device);
    void DestroyView(VkDevice device);

private:
    UnityVulkanImage image_{};
    VkImageView view_ = VK_NULL_HANDLE;
};
