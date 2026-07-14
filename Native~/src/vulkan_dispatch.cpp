#include "vulkan_dispatch.h"

VulkanDispatch& VulkanDispatch::Instance()
{
    static VulkanDispatch dispatch;
    return dispatch;
}

bool VulkanDispatch::Load(
    VkInstance instance,
    VkDevice device,
    PFN_vkGetInstanceProcAddr getInstanceProcAddr)
{
    Reset();

    if (instance == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        getInstanceProcAddr == nullptr)
    {
        return false;
    }

    const auto getDeviceProcAddr =
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(
            getInstanceProcAddr(
                instance,
                "vkGetDeviceProcAddr"));

    if (getDeviceProcAddr == nullptr)
        return false;

    instance_ = instance;
    device_ = device;

#define LOAD_DEVICE_FUNCTION(member, name)                       \
    member = reinterpret_cast<decltype(member)>(                 \
        getDeviceProcAddr(device_, name));                       \
    if (member == nullptr)                                       \
    {                                                            \
        Reset();                                                 \
        return false;                                            \
    }

    LOAD_DEVICE_FUNCTION(
        getBufferDeviceAddress,
        "vkGetBufferDeviceAddressKHR");

    LOAD_DEVICE_FUNCTION(
        createAccelerationStructure,
        "vkCreateAccelerationStructureKHR");

    LOAD_DEVICE_FUNCTION(
        destroyAccelerationStructure,
        "vkDestroyAccelerationStructureKHR");

    LOAD_DEVICE_FUNCTION(
        getAccelerationStructureBuildSizes,
        "vkGetAccelerationStructureBuildSizesKHR");

    LOAD_DEVICE_FUNCTION(
        cmdBuildAccelerationStructures,
        "vkCmdBuildAccelerationStructuresKHR");

    LOAD_DEVICE_FUNCTION(
        getAccelerationStructureDeviceAddress,
        "vkGetAccelerationStructureDeviceAddressKHR");

    LOAD_DEVICE_FUNCTION(
        createRayTracingPipelines,
        "vkCreateRayTracingPipelinesKHR");

    LOAD_DEVICE_FUNCTION(
        getRayTracingShaderGroupHandles,
        "vkGetRayTracingShaderGroupHandlesKHR");

    LOAD_DEVICE_FUNCTION(
        cmdTraceRays,
        "vkCmdTraceRaysKHR");

#undef LOAD_DEVICE_FUNCTION

    loaded_ = true;
    return true;
}

void VulkanDispatch::Reset()
{
    instance_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    loaded_ = false;

    getBufferDeviceAddress = nullptr;
    createAccelerationStructure = nullptr;
    destroyAccelerationStructure = nullptr;
    getAccelerationStructureBuildSizes = nullptr;
    cmdBuildAccelerationStructures = nullptr;
    getAccelerationStructureDeviceAddress = nullptr;
    createRayTracingPipelines = nullptr;
    getRayTracingShaderGroupHandles = nullptr;
    cmdTraceRays = nullptr;
}

bool VulkanDispatch::IsLoaded() const
{
    return loaded_;
}
