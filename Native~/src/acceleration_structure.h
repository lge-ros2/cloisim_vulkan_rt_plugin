#pragma once

#include "gpu_buffer.h"
#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

class AccelerationStructure
{
public:
    AccelerationStructure() = default;
    ~AccelerationStructure();

    AccelerationStructure(const AccelerationStructure&) = delete;
    AccelerationStructure& operator=(const AccelerationStructure&) = delete;

    AccelerationStructure(AccelerationStructure&& other) noexcept;
    AccelerationStructure& operator=(AccelerationStructure&& other) noexcept;

    bool Create(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch,
        VkAccelerationStructureTypeKHR type,
        VkDeviceSize size);

    void Destroy();

    VkAccelerationStructureKHR Handle() const { return handle_; }
    VkDeviceAddress DeviceAddress() const { return deviceAddress_; }
    VkAccelerationStructureTypeKHR Type() const { return type_; }
    bool IsValid() const
    {
        return handle_ != VK_NULL_HANDLE && storage_.IsValid();
    }

private:
    void MoveFrom(AccelerationStructure&& other) noexcept;

    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
    GpuBuffer storage_;
    VkAccelerationStructureKHR handle_ = VK_NULL_HANDLE;
    VkDeviceAddress deviceAddress_ = 0;
    VkAccelerationStructureTypeKHR type_ =
        VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
};
