#include "unity_output_texture.h"

bool UnityOutputTexture::AccessForStorage(
    IUnityGraphicsVulkanV2* vulkan,
    void* nativeTexture)
{
    Reset();

    if (vulkan == nullptr || nativeTexture == nullptr)
        return false;

    return vulkan->AccessTexture(
        nativeTexture,
        UnityVulkanWholeImage,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_ACCESS_SHADER_WRITE_BIT,
        kUnityVulkanResourceAccess_PipelineBarrier,
        &image_);
}

bool UnityOutputTexture::EnsureView(VkDevice device)
{
    if (view_ != VK_NULL_HANDLE)
        return true;

    if (device == VK_NULL_HANDLE || image_.image == VK_NULL_HANDLE)
        return false;

    VkImageViewCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    info.image = image_.image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = image_.format;
    info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    return vkCreateImageView(device, &info, nullptr, &view_) == VK_SUCCESS;
}

void UnityOutputTexture::DestroyView(VkDevice device)
{
    if (device != VK_NULL_HANDLE && view_ != VK_NULL_HANDLE)
        vkDestroyImageView(device, view_, nullptr);
    view_ = VK_NULL_HANDLE;
}

void UnityOutputTexture::Reset()
{
    image_ = {};
}
