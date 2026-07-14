#pragma once

#include "IUnityInterface.h"
#include "cloisim_vulkan_rt/api.h"

#include <vulkan/vulkan.h>

class VulkanInterceptor
{
public:
    static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API
    InitializationCallback(
        PFN_vkGetInstanceProcAddr next,
        void* userData);

    static void FillCapabilities(
        CloiSimRtCapabilities& capabilities);

    static bool NativeBackendAvailable();

private:
    static PFN_vkVoidFunction VKAPI_PTR
    GetInstanceProcAddr(
        VkInstance instance,
        const char* name);

    static VkResult VKAPI_PTR
    CreateDevice(
        VkPhysicalDevice physicalDevice,
        const VkDeviceCreateInfo* createInfo,
        const VkAllocationCallbacks* allocator,
        VkDevice* device);
};
