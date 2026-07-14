#include "gpu_buffer.h"
#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <vector>

namespace
{
constexpr int kSkip = 77;

bool HasExtension(
    const std::vector<VkExtensionProperties>& properties,
    const char* name)
{
    return std::any_of(
        properties.begin(),
        properties.end(),
        [name](const VkExtensionProperties& property)
        {
            return std::strcmp(property.extensionName, name) == 0;
        });
}

int Skip(const char* reason)
{
    std::cout << "[SKIP] " << reason << '\n';
    return kSkip;
}

int Fail(const char* reason)
{
    std::cerr << "[FAIL] " << reason << '\n';
    return 1;
}
}

int main()
{
    VkApplicationInfo application{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    application.pApplicationName = "cloisim_rt_gpu_device_test";
    application.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo{
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &application;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
        return Skip("Vulkan 1.2 instance is unavailable");

    uint32_t physicalDeviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    if (physicalDeviceCount == 0)
    {
        vkDestroyInstance(instance, nullptr);
        return Skip("no Vulkan physical device");
    }

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(
        instance,
        &physicalDeviceCount,
        physicalDevices.data());

    VkPhysicalDevice selected = VK_NULL_HANDLE;
    uint32_t queueFamilyIndex = 0;

    const std::array<const char*, 4> requiredExtensions = {
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    for (VkPhysicalDevice candidate : physicalDevices)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(
            candidate,
            nullptr,
            &extensionCount,
            nullptr);
        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            candidate,
            nullptr,
            &extensionCount,
            extensions.data());

        if (!std::all_of(
                requiredExtensions.begin(),
                requiredExtensions.end(),
                [&extensions](const char* name)
                {
                    return HasExtension(extensions, name);
                }))
        {
            continue;
        }

        VkPhysicalDeviceBufferDeviceAddressFeatures bda{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracing{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
        VkPhysicalDeviceFeatures2 features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features.pNext = &bda;
        bda.pNext = &acceleration;
        acceleration.pNext = &rayTracing;
        vkGetPhysicalDeviceFeatures2(candidate, &features);

        if (!bda.bufferDeviceAddress ||
            !acceleration.accelerationStructure ||
            !rayTracing.rayTracingPipeline)
        {
            continue;
        }

        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate,
            &queueCount,
            nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate,
            &queueCount,
            queues.data());

        for (uint32_t index = 0; index < queueCount; ++index)
        {
            if ((queues[index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
            {
                selected = candidate;
                queueFamilyIndex = index;
                break;
            }
        }

        if (selected != VK_NULL_HANDLE)
            break;
    }

    if (selected == VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        return Skip("no device exposes required KHR ray tracing features");
    }

    float priority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceBufferDeviceAddressFeatures bda{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    bda.bufferDeviceAddress = VK_TRUE;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    acceleration.accelerationStructure = VK_TRUE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracing{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rayTracing.rayTracingPipeline = VK_TRUE;
    bda.pNext = &acceleration;
    acceleration.pNext = &rayTracing;

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.pNext = &bda;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = requiredExtensions.size();
    deviceInfo.ppEnabledExtensionNames = requiredExtensions.data();

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(selected, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        vkDestroyInstance(instance, nullptr);
        return Fail("vkCreateDevice failed");
    }

    auto gipa = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        vkGetInstanceProcAddr(instance, "vkGetInstanceProcAddr"));
    auto& dispatch = VulkanDispatch::Instance();
    if (!dispatch.Load(instance, device, gipa))
    {
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return Fail("ray tracing dispatch load failed");
    }

    GpuBuffer buffer;
    const bool created = buffer.Create(
        selected,
        device,
        &dispatch,
        4096,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    if (!created || buffer.DeviceAddress() == 0)
    {
        buffer.Destroy();
        dispatch.Reset();
        vkDestroyDevice(device, nullptr);
        vkDestroyInstance(instance, nullptr);
        return Fail("device-address buffer creation failed");
    }

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(selected, &properties);
    std::cout << "[OK] GPU=" << properties.deviceName
              << " api=" << VK_API_VERSION_MAJOR(properties.apiVersion)
              << '.' << VK_API_VERSION_MINOR(properties.apiVersion)
              << " address=0x" << std::hex << buffer.DeviceAddress()
              << std::dec << '\n';

    buffer.Destroy();
    dispatch.Reset();
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    return 0;
}
