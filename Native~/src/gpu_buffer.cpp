#include "gpu_buffer.h"

#include "vulkan_memory.h"

#include <cstring>
#include <utility>

GpuBuffer::~GpuBuffer()
{
    Destroy();
}

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
{
    MoveFrom(std::move(other));
}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        MoveFrom(std::move(other));
    }
    return *this;
}

bool GpuBuffer::Create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags requiredMemoryProperties,
    VkMemoryPropertyFlags preferredMemoryProperties)
{
    Destroy();

    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        size == 0)
    {
        return false;
    }

    const bool needsDeviceAddress =
        (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) != 0;

    if (needsDeviceAddress &&
        (dispatch == nullptr || dispatch->getBufferDeviceAddress == nullptr))
    {
        return false;
    }

    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateBuffer(
        device,
        &bufferInfo,
        nullptr,
        &buffer_);
    if (result != VK_SUCCESS)
        return false;

    VkMemoryRequirements requirements{};
    vkGetBufferMemoryRequirements(device, buffer_, &requirements);

    result = VulkanMemory::Allocate(
        physicalDevice,
        device,
        requirements,
        requiredMemoryProperties,
        preferredMemoryProperties,
        needsDeviceAddress,
        memory_,
        memoryProperties_);
    if (result != VK_SUCCESS)
    {
        vkDestroyBuffer(device, buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        return false;
    }

    result = vkBindBufferMemory(device, buffer_, memory_, 0);
    if (result != VK_SUCCESS)
    {
        vkFreeMemory(device, memory_, nullptr);
        vkDestroyBuffer(device, buffer_, nullptr);
        memory_ = VK_NULL_HANDLE;
        buffer_ = VK_NULL_HANDLE;
        memoryProperties_ = 0;
        return false;
    }

    device_ = device;
    dispatch_ = dispatch;
    size_ = size;

    if (needsDeviceAddress)
    {
        VkBufferDeviceAddressInfo addressInfo{
            VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
        addressInfo.buffer = buffer_;
        deviceAddress_ = dispatch_->getBufferDeviceAddress(
            device_,
            &addressInfo);

        if (deviceAddress_ == 0)
        {
            Destroy();
            return false;
        }
    }

    return true;
}

bool GpuBuffer::Upload(
    const void* source,
    VkDeviceSize size,
    VkDeviceSize offset)
{
    if (!IsValid() || source == nullptr || size == 0)
        return false;

    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
        return false;

    if (offset > size_ || size > size_ - offset)
        return false;

    void* mapped = nullptr;
    VkResult result = vkMapMemory(
        device_,
        memory_,
        offset,
        size,
        0,
        &mapped);
    if (result != VK_SUCCESS)
        return false;

    std::memcpy(mapped, source, static_cast<std::size_t>(size));

    if ((memoryProperties_ & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
    {
        VkMappedMemoryRange range{
            VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
        range.memory = memory_;
        range.offset = offset;
        range.size = size;
        result = vkFlushMappedMemoryRanges(device_, 1, &range);
    }

    vkUnmapMemory(device_, memory_);
    return result == VK_SUCCESS;
}

void GpuBuffer::Destroy()
{
    if (device_ != VK_NULL_HANDLE)
    {
        if (buffer_ != VK_NULL_HANDLE)
            vkDestroyBuffer(device_, buffer_, nullptr);
        if (memory_ != VK_NULL_HANDLE)
            vkFreeMemory(device_, memory_, nullptr);
    }

    device_ = VK_NULL_HANDLE;
    dispatch_ = nullptr;
    buffer_ = VK_NULL_HANDLE;
    memory_ = VK_NULL_HANDLE;
    size_ = 0;
    deviceAddress_ = 0;
    memoryProperties_ = 0;
}

void GpuBuffer::MoveFrom(GpuBuffer&& other) noexcept
{
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    dispatch_ = std::exchange(other.dispatch_, nullptr);
    buffer_ = std::exchange(other.buffer_, VK_NULL_HANDLE);
    memory_ = std::exchange(other.memory_, VK_NULL_HANDLE);
    size_ = std::exchange(other.size_, 0);
    deviceAddress_ = std::exchange(other.deviceAddress_, 0);
    memoryProperties_ = std::exchange(other.memoryProperties_, 0);
}
