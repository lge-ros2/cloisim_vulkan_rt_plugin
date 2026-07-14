#pragma once

#include <vulkan/vulkan.h>

class VulkanDispatch
{
public:
    static VulkanDispatch& Instance();

    bool Load(
        VkInstance instance,
        VkDevice device,
        PFN_vkGetInstanceProcAddr getInstanceProcAddr);

    void Reset();
    bool IsLoaded() const;

    PFN_vkGetBufferDeviceAddressKHR
        getBufferDeviceAddress = nullptr;

    PFN_vkCreateAccelerationStructureKHR
        createAccelerationStructure = nullptr;

    PFN_vkDestroyAccelerationStructureKHR
        destroyAccelerationStructure = nullptr;

    PFN_vkGetAccelerationStructureBuildSizesKHR
        getAccelerationStructureBuildSizes = nullptr;

    PFN_vkCmdBuildAccelerationStructuresKHR
        cmdBuildAccelerationStructures = nullptr;

    PFN_vkGetAccelerationStructureDeviceAddressKHR
        getAccelerationStructureDeviceAddress = nullptr;

    PFN_vkCreateRayTracingPipelinesKHR
        createRayTracingPipelines = nullptr;

    PFN_vkGetRayTracingShaderGroupHandlesKHR
        getRayTracingShaderGroupHandles = nullptr;

    PFN_vkCmdTraceRaysKHR
        cmdTraceRays = nullptr;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    bool loaded_ = false;
};
