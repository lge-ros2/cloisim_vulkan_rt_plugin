#include "unity_vulkan_bridge.h"

#include "cloisim_vulkan_rt/api.h"
#include "vulkan_interceptor.h"
#include "vulkan_dispatch.h"
#include "rt_runtime_context.h"

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
        RtRuntimeContext::Instance().Shutdown();
        VulkanDispatch::Instance().Reset();
        instance_ = {};
        initialized_ = false;
        return;
    }

    if (eventType != kUnityGfxDeviceEventInitialize || vulkan_ == nullptr)
        return;

    instance_ = vulkan_->Instance();
    initialized_ = instance_.device != VK_NULL_HANDLE;

    if (initialized_)
    {
        const bool dispatchLoaded = VulkanDispatch::Instance().Load(
            instance_.instance,
            instance_.device,
            instance_.getInstanceProcAddr);

        if (dispatchLoaded)
        {
            RtRuntimeContext::Instance().Initialize(
                instance_.physicalDevice,
                instance_.device,
                &VulkanDispatch::Instance());
        }
    }
}

bool UnityVulkanBridge::GetCapabilities(CloiSimRtCapabilities* capabilities) const
{
    if (!initialized_ || capabilities == nullptr)
        return false;

    std::memset(capabilities, 0, sizeof(*capabilities));
    capabilities->abiVersion = 2;
    capabilities->pluginLoaded = 1;
    capabilities->vulkanDeviceReady = 1;
    capabilities->dispatchLoaded =
        VulkanDispatch::Instance().IsLoaded() ? 1U : 0U;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(instance_.physicalDevice, &properties);

    capabilities->apiVersion = properties.apiVersion;
    capabilities->vendorId = properties.vendorID;
    capabilities->deviceId = properties.deviceID;

    auto& runtime = RtRuntimeContext::Instance();
    if (runtime.IsDepthPipelineReady())
    {
        const auto& rtProperties =
            runtime.DepthPipeline().Pipeline().Properties();
        capabilities->maxRecursionDepth =
            rtProperties.maxRayRecursionDepth;
        capabilities->shaderGroupHandleSize =
            rtProperties.shaderGroupHandleSize;
        capabilities->shaderGroupBaseAlignment =
            rtProperties.shaderGroupBaseAlignment;
    }

    VulkanInterceptor::FillCapabilities(*capabilities);
    return true;
}

IUnityGraphicsVulkanV2* UnityVulkanBridge::Vulkan() const
{
    return vulkan_;
}
