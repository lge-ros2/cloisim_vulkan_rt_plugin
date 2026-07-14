#pragma once

#include "gpu_buffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

class TlasInstanceBuffer
{
public:
    bool CreateAndUpload(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch,
        const std::vector<VkAccelerationStructureInstanceKHR>& instances);

    void Destroy();

    VkBuffer Handle() const { return buffer_.Handle(); }
    VkDeviceAddress DeviceAddress() const
    {
        return buffer_.DeviceAddress();
    }
    uint32_t InstanceCount() const { return instanceCount_; }
    bool IsValid() const
    {
        return buffer_.IsValid() && instanceCount_ != 0;
    }

    GpuBuffer ReleaseBuffer();

private:
    GpuBuffer buffer_;
    uint32_t instanceCount_ = 0;
};
