#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

class VulkanMemory
{
public:
    static bool FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t memoryTypeBits,
        VkMemoryPropertyFlags requiredProperties,
        VkMemoryPropertyFlags preferredProperties,
        uint32_t& memoryTypeIndex,
        VkMemoryPropertyFlags& selectedProperties);

    static VkResult Allocate(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        const VkMemoryRequirements& requirements,
        VkMemoryPropertyFlags requiredProperties,
        VkMemoryPropertyFlags preferredProperties,
        bool deviceAddress,
        VkDeviceMemory& memory,
        VkMemoryPropertyFlags& selectedProperties);
};
