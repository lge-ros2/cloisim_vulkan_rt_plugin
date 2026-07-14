#pragma once

#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

#include <cstddef>

class GpuBuffer
{
public:
    GpuBuffer() = default;
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;

    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;

    bool Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags requiredMemoryProperties,
        VkMemoryPropertyFlags preferredMemoryProperties = 0);

    bool Upload(
        const void* source,
        VkDeviceSize size,
        VkDeviceSize offset = 0);

    void Destroy();

    VkBuffer Handle() const { return buffer_; }
    VkDeviceMemory Memory() const { return memory_; }
    VkDeviceSize Size() const { return size_; }
    VkDeviceAddress DeviceAddress() const { return deviceAddress_; }
    VkMemoryPropertyFlags MemoryProperties() const
    {
        return memoryProperties_;
    }
    bool IsValid() const
    {
        return buffer_ != VK_NULL_HANDLE && memory_ != VK_NULL_HANDLE;
    }

private:
    void MoveFrom(GpuBuffer&& other) noexcept;

    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkDeviceSize size_ = 0;
    VkDeviceAddress deviceAddress_ = 0;
    VkMemoryPropertyFlags memoryProperties_ = 0;
};
