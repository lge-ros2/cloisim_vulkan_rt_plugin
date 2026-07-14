#include "vulkan_interceptor.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace
{
PFN_vkGetInstanceProcAddr g_nextGipa = nullptr;
PFN_vkCreateDevice g_nextCreateDevice = nullptr;
PFN_vkEnumerateDeviceExtensionProperties g_enumerateDeviceExtensions = nullptr;
PFN_vkGetPhysicalDeviceFeatures2 g_getPhysicalDeviceFeatures2 = nullptr;

bool g_hasAccelerationStructure = false;
bool g_hasRayTracingPipeline = false;
bool g_hasRayQuery = false;
bool g_hasBufferDeviceAddress = false;
bool g_deviceCreatedWithRt = false;

bool HasExtension(
    const std::vector<VkExtensionProperties>& extensions,
    const char* name)
{
    return std::any_of(
        extensions.begin(),
        extensions.end(),
        [name](const VkExtensionProperties& extension)
        {
            return std::strcmp(extension.extensionName, name) == 0;
        });
}

void AddExtension(
    std::vector<const char*>& extensions,
    const char* name)
{
    const auto found = std::find_if(
        extensions.begin(),
        extensions.end(),
        [name](const char* extension)
        {
            return std::strcmp(extension, name) == 0;
        });

    if (found == extensions.end())
        extensions.push_back(name);
}

VkBaseOutStructure* FindStructure(
    const void* chain,
    VkStructureType type)
{
    auto* current = reinterpret_cast<VkBaseOutStructure*>(
        const_cast<void*>(chain));

    while (current != nullptr)
    {
        if (current->sType == type)
            return current;

        current = current->pNext;
    }

    return nullptr;
}
}

PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API
VulkanInterceptor::InitializationCallback(
    PFN_vkGetInstanceProcAddr next,
    void*)
{
    g_nextGipa = next;
    return GetInstanceProcAddr;
}

PFN_vkVoidFunction VKAPI_PTR
VulkanInterceptor::GetInstanceProcAddr(
    VkInstance instance,
    const char* name)
{
    if (g_nextGipa == nullptr || name == nullptr)
        return nullptr;

    if (instance != VK_NULL_HANDLE)
    {
        if (g_enumerateDeviceExtensions == nullptr)
        {
            g_enumerateDeviceExtensions =
                reinterpret_cast<PFN_vkEnumerateDeviceExtensionProperties>(
                    g_nextGipa(
                        instance,
                        "vkEnumerateDeviceExtensionProperties"));
        }

        if (g_getPhysicalDeviceFeatures2 == nullptr)
        {
            g_getPhysicalDeviceFeatures2 =
                reinterpret_cast<PFN_vkGetPhysicalDeviceFeatures2>(
                    g_nextGipa(
                        instance,
                        "vkGetPhysicalDeviceFeatures2"));
        }
    }

    if (std::strcmp(name, "vkCreateDevice") == 0)
    {
        g_nextCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(
            g_nextGipa(instance, name));

        return reinterpret_cast<PFN_vkVoidFunction>(CreateDevice);
    }

    return g_nextGipa(instance, name);
}

VkResult VKAPI_PTR VulkanInterceptor::CreateDevice(
    VkPhysicalDevice physicalDevice,
    const VkDeviceCreateInfo* source,
    const VkAllocationCallbacks* allocator,
    VkDevice* device)
{
    g_deviceCreatedWithRt = false;

    if (g_nextCreateDevice == nullptr || source == nullptr)
        return VK_ERROR_INITIALIZATION_FAILED;

    if (g_enumerateDeviceExtensions == nullptr ||
        g_getPhysicalDeviceFeatures2 == nullptr)
    {
        return g_nextCreateDevice(
            physicalDevice,
            source,
            allocator,
            device);
    }

    uint32_t extensionCount = 0;
    VkResult result = g_enumerateDeviceExtensions(
        physicalDevice,
        nullptr,
        &extensionCount,
        nullptr);

    if (result != VK_SUCCESS)
    {
        return g_nextCreateDevice(
            physicalDevice,
            source,
            allocator,
            device);
    }

    std::vector<VkExtensionProperties> available(extensionCount);
    result = g_enumerateDeviceExtensions(
        physicalDevice,
        nullptr,
        &extensionCount,
        available.data());

    if (result != VK_SUCCESS)
    {
        return g_nextCreateDevice(
            physicalDevice,
            source,
            allocator,
            device);
    }

    g_hasAccelerationStructure = HasExtension(
        available,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    g_hasRayTracingPipeline = HasExtension(
        available,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

    g_hasRayQuery = HasExtension(
        available,
        VK_KHR_RAY_QUERY_EXTENSION_NAME);

    g_hasBufferDeviceAddress = HasExtension(
        available,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    const bool hasDeferredHostOperations = HasExtension(
        available,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    if (!g_hasAccelerationStructure ||
        !g_hasRayTracingPipeline ||
        !g_hasBufferDeviceAddress ||
        !hasDeferredHostOperations)
    {
        return g_nextCreateDevice(
            physicalDevice,
            source,
            allocator,
            device);
    }

    VkPhysicalDeviceBufferDeviceAddressFeatures supportedBda{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};

    VkPhysicalDeviceAccelerationStructureFeaturesKHR supportedAs{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR supportedRt{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};

    VkPhysicalDeviceRayQueryFeaturesKHR supportedRq{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};

    VkPhysicalDeviceFeatures2 supported{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    supported.pNext = &supportedBda;
    supportedBda.pNext = &supportedAs;
    supportedAs.pNext = &supportedRt;
    supportedRt.pNext = &supportedRq;

    g_getPhysicalDeviceFeatures2(physicalDevice, &supported);

    if (supportedBda.bufferDeviceAddress != VK_TRUE ||
        supportedAs.accelerationStructure != VK_TRUE ||
        supportedRt.rayTracingPipeline != VK_TRUE)
    {
        return g_nextCreateDevice(
            physicalDevice,
            source,
            allocator,
            device);
    }

    std::vector<const char*> extensions(
        source->ppEnabledExtensionNames,
        source->ppEnabledExtensionNames + source->enabledExtensionCount);

    AddExtension(
        extensions,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME);

    AddExtension(
        extensions,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);

    AddExtension(
        extensions,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);

    AddExtension(
        extensions,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    if (g_hasRayQuery && supportedRq.rayQuery == VK_TRUE)
        AddExtension(extensions, VK_KHR_RAY_QUERY_EXTENSION_NAME);

    VkPhysicalDeviceBufferDeviceAddressFeatures injectedBda{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};

    VkPhysicalDeviceAccelerationStructureFeaturesKHR injectedAs{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR injectedRt{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};

    VkPhysicalDeviceRayQueryFeaturesKHR injectedRq{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR};

    void* featureChain = const_cast<void*>(source->pNext);

    auto* existingBda = reinterpret_cast<
        VkPhysicalDeviceBufferDeviceAddressFeatures*>(
            FindStructure(
                source->pNext,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES));

    if (existingBda != nullptr)
    {
        existingBda->bufferDeviceAddress = VK_TRUE;
    }
    else
    {
        injectedBda.bufferDeviceAddress = VK_TRUE;
        injectedBda.pNext = featureChain;
        featureChain = &injectedBda;
    }

    auto* existingAs = reinterpret_cast<
        VkPhysicalDeviceAccelerationStructureFeaturesKHR*>(
            FindStructure(
                source->pNext,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR));

    if (existingAs != nullptr)
    {
        existingAs->accelerationStructure = VK_TRUE;
    }
    else
    {
        injectedAs.accelerationStructure = VK_TRUE;
        injectedAs.pNext = featureChain;
        featureChain = &injectedAs;
    }

    auto* existingRt = reinterpret_cast<
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR*>(
            FindStructure(
                source->pNext,
                VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR));

    if (existingRt != nullptr)
    {
        existingRt->rayTracingPipeline = VK_TRUE;
    }
    else
    {
        injectedRt.rayTracingPipeline = VK_TRUE;
        injectedRt.pNext = featureChain;
        featureChain = &injectedRt;
    }

    if (g_hasRayQuery &&
        supportedRq.rayQuery == VK_TRUE)
    {
        auto* existingRq = reinterpret_cast<
            VkPhysicalDeviceRayQueryFeaturesKHR*>(
                FindStructure(
                    source->pNext,
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR));

        if (existingRq != nullptr)
        {
            existingRq->rayQuery = VK_TRUE;
        }
        else
        {
            injectedRq.rayQuery = VK_TRUE;
            injectedRq.pNext = featureChain;
            featureChain = &injectedRq;
        }
    }

    VkDeviceCreateInfo createInfo = *source;

    createInfo.enabledExtensionCount =
        static_cast<uint32_t>(extensions.size());

    createInfo.ppEnabledExtensionNames =
        extensions.data();

    createInfo.pNext = featureChain;

    result = g_nextCreateDevice(
        physicalDevice,
        &createInfo,
        allocator,
        device);

    g_deviceCreatedWithRt =
        result == VK_SUCCESS;

    return result;
}

void VulkanInterceptor::FillCapabilities(
    CloiSimRtCapabilities& capabilities)
{
    capabilities.accelerationStructure =
        g_hasAccelerationStructure ? 1U : 0U;

    capabilities.rayTracingPipeline =
        g_hasRayTracingPipeline ? 1U : 0U;

    capabilities.rayQuery =
        g_hasRayQuery ? 1U : 0U;

    capabilities.bufferDeviceAddress =
        g_hasBufferDeviceAddress ? 1U : 0U;
}

bool VulkanInterceptor::NativeBackendAvailable()
{
    return g_deviceCreatedWithRt;
}
