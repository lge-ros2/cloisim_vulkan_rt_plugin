#include "acceleration_structure.h"

#include <utility>

AccelerationStructure::~AccelerationStructure()
{
    Destroy();
}

AccelerationStructure::AccelerationStructure(
    AccelerationStructure&& other) noexcept
{
    MoveFrom(std::move(other));
}

AccelerationStructure& AccelerationStructure::operator=(
    AccelerationStructure&& other) noexcept
{
    if (this != &other)
    {
        Destroy();
        MoveFrom(std::move(other));
    }
    return *this;
}

bool AccelerationStructure::Create(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch,
    VkAccelerationStructureTypeKHR type,
    VkDeviceSize size)
{
    Destroy();

    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        dispatch == nullptr ||
        !dispatch->IsLoaded() ||
        size == 0)
    {
        return false;
    }

    const VkBufferUsageFlags usage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!storage_.Create(
            physicalDevice,
            device,
            dispatch,
            size,
            usage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        return false;
    }

    VkAccelerationStructureCreateInfoKHR createInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR};
    createInfo.buffer = storage_.Handle();
    createInfo.offset = 0;
    createInfo.size = size;
    createInfo.type = type;

    const VkResult result = dispatch->createAccelerationStructure(
        device,
        &createInfo,
        nullptr,
        &handle_);
    if (result != VK_SUCCESS)
    {
        storage_.Destroy();
        return false;
    }

    device_ = device;
    dispatch_ = dispatch;
    type_ = type;

    VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR};
    addressInfo.accelerationStructure = handle_;
    deviceAddress_ = dispatch_->getAccelerationStructureDeviceAddress(
        device_,
        &addressInfo);

    if (deviceAddress_ == 0)
    {
        Destroy();
        return false;
    }

    return true;
}

void AccelerationStructure::Destroy()
{
    if (device_ != VK_NULL_HANDLE &&
        dispatch_ != nullptr &&
        handle_ != VK_NULL_HANDLE)
    {
        dispatch_->destroyAccelerationStructure(
            device_,
            handle_,
            nullptr);
    }

    handle_ = VK_NULL_HANDLE;
    deviceAddress_ = 0;
    type_ = VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR;
    storage_.Destroy();
    dispatch_ = nullptr;
    device_ = VK_NULL_HANDLE;
}

void AccelerationStructure::MoveFrom(
    AccelerationStructure&& other) noexcept
{
    device_ = std::exchange(other.device_, VK_NULL_HANDLE);
    dispatch_ = std::exchange(other.dispatch_, nullptr);
    storage_ = std::move(other.storage_);
    handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
    deviceAddress_ = std::exchange(other.deviceAddress_, 0);
    type_ = std::exchange(
        other.type_,
        VK_ACCELERATION_STRUCTURE_TYPE_GENERIC_KHR);
}
