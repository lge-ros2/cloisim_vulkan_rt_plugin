#include "unity_vulkan_bridge.h"

#include "cloisim_vulkan_rt/api.h"
#include "vulkan_interceptor.h"

#include <cstring>

UnityVulkanBridge& UnityVulkanBridge::Instance()
{
    static UnityVulkanBridge bridge;
    return bridge;
}

void UnityVulkanBridge::Load(IUnityInterfaces* interfaces)
{
    vulkan_ = interfaces->Get<IUnityGraphicsVulkanV2>();
}

bool UnityVulkanBridge::InstallInitializationInterceptor()
{
    return vulkan_ != nullptr && vulkan_->InterceptInitialization(
        VulkanInterceptor::InitializationCallback, nullptr);
}

void UnityVulkanBridge::OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        instance_ = {};
        initialized_ = false;
        return;
    }

    if (eventType != kUnityGfxDeviceEventInitialize || vulkan_ == nullptr)
        return;

    instance_ = vulkan_->Instance();
    initialized_ = instance_.device != VK_NULL_HANDLE;
}

bool UnityVulkanBridge::GetCapabilities(CloiSimRtCapabilities* capabilities) const
{
    if (!initialized_ || capabilities == nullptr)
        return false;

    std::memset(capabilities, 0, sizeof(*capabilities));
    capabilities->abiVersion = 1;
    capabilities->pluginLoaded = 1;
    capabilities->vulkanDeviceReady = 1;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(instance_.physicalDevice, &properties);

    capabilities->apiVersion = properties.apiVersion;
    capabilities->vendorId = properties.vendorID;
    capabilities->deviceId = properties.deviceID;

    VulkanInterceptor::FillCapabilities(*capabilities);
    return true;
}

IUnityGraphicsVulkanV2* UnityVulkanBridge::Vulkan() const
{
    return vulkan_;
}
