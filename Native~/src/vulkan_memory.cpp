#include "vulkan_memory.h"

namespace
{
uint32_t CountBits(uint32_t value)
{
    uint32_t count = 0;
    while (value != 0)
    {
        count += value & 1U;
        value >>= 1U;
    }
    return count;
}
}

bool VulkanMemory::FindMemoryType(
    VkPhysicalDevice physicalDevice,
    uint32_t memoryTypeBits,
    VkMemoryPropertyFlags requiredProperties,
    VkMemoryPropertyFlags preferredProperties,
    uint32_t& memoryTypeIndex,
    VkMemoryPropertyFlags& selectedProperties)
{
    VkPhysicalDeviceMemoryProperties properties{};
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &properties);

    bool found = false;
    uint32_t bestScore = 0;

    for (uint32_t index = 0; index < properties.memoryTypeCount; ++index)
    {
        if ((memoryTypeBits & (1U << index)) == 0)
            continue;

        const VkMemoryPropertyFlags flags =
            properties.memoryTypes[index].propertyFlags;

        if ((flags & requiredProperties) != requiredProperties)
            continue;

        const uint32_t score = CountBits(flags & preferredProperties);
        if (!found || score > bestScore)
        {
            found = true;
            bestScore = score;
            memoryTypeIndex = index;
            selectedProperties = flags;
        }
    }

    return found;
}

VkResult VulkanMemory::Allocate(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    const VkMemoryRequirements& requirements,
    VkMemoryPropertyFlags requiredProperties,
    VkMemoryPropertyFlags preferredProperties,
    bool deviceAddress,
    VkDeviceMemory& memory,
    VkMemoryPropertyFlags& selectedProperties)
{
    memory = VK_NULL_HANDLE;
    selectedProperties = 0;

    uint32_t memoryTypeIndex = 0;
    if (!FindMemoryType(
            physicalDevice,
            requirements.memoryTypeBits,
            requiredProperties,
            preferredProperties,
            memoryTypeIndex,
            selectedProperties))
    {
        return VK_ERROR_FEATURE_NOT_PRESENT;
    }

    VkMemoryAllocateFlagsInfo flagsInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO};
    if (deviceAddress)
        flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

    VkMemoryAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocateInfo.pNext = deviceAddress ? &flagsInfo : nullptr;
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = memoryTypeIndex;

    return vkAllocateMemory(device, &allocateInfo, nullptr, &memory);
}
