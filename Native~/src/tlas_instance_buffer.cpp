#include "tlas_instance_buffer.h"

#include <limits>
#include <utility>

bool TlasInstanceBuffer::CreateAndUpload(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch,
    const std::vector<VkAccelerationStructureInstanceKHR>& instances)
{
    Destroy();

    if (instances.empty() ||
        instances.size() > std::numeric_limits<uint32_t>::max())
    {
        return false;
    }

    const VkDeviceSize size =
        sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

    const VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!buffer_.Create(
            physicalDevice,
            device,
            dispatch,
            size,
            usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
    {
        return false;
    }

    if (!buffer_.Upload(instances.data(), size))
    {
        buffer_.Destroy();
        return false;
    }

    instanceCount_ = static_cast<uint32_t>(instances.size());
    return true;
}

void TlasInstanceBuffer::Destroy()
{
    buffer_.Destroy();
    instanceCount_ = 0;
}

GpuBuffer TlasInstanceBuffer::ReleaseBuffer()
{
    instanceCount_ = 0;
    return std::move(buffer_);
}
