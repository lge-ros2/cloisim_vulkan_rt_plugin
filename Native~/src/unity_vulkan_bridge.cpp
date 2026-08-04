#include "unity_vulkan_bridge.h"

#include "cloisim_vulkan_rt/api.h"
#include "vulkan_interceptor.h"
#include "vulkan_dispatch.h"
#include "rt_runtime_context.h"

#include <cstdio>
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
    const bool installed =
        vulkan_ != nullptr && vulkan_->InterceptInitialization(
            VulkanInterceptor::InitializationCallback, nullptr);

    std::fprintf(
        stderr,
        "[CLOiSimRt] InstallInitializationInterceptor: vulkan_=%p "
        "installed=%d\n",
        reinterpret_cast<void*>(vulkan_),
        installed);

    return installed;
}

void UnityVulkanBridge::OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    std::fprintf(
        stderr,
        "[CLOiSimRt] OnGraphicsDeviceEvent: eventType=%d\n",
        static_cast<int>(eventType));

    if (eventType == kUnityGfxDeviceEventShutdown)
    {
        RtRuntimeContext::Instance().Shutdown();
        VulkanDispatch::Instance().Reset();
        instance_ = {};
        initialized_ = false;
        deviceEventSeen_ = false;
        return;
    }

    if (eventType != kUnityGfxDeviceEventInitialize || vulkan_ == nullptr)
        return;

    // Do not query vulkan_->Instance() here: with a vkCreateDevice
    // interceptor installed, this event fires synchronously from within
    // Unity's own device bring-up call stack, before its UnityVulkanInstance
    // bookkeeping is fully populated. The actual query is deferred to
    // EnsureReady(), invoked lazily from managed-code entry points.
    deviceEventSeen_ = true;

    std::fprintf(
        stderr,
        "[CLOiSimRt] OnGraphicsDeviceEvent: device create event seen, "
        "deferring Instance() query\n");
}

bool UnityVulkanBridge::EnsureReady()
{
    if (initialized_)
        return true;

    if (!deviceEventSeen_ || vulkan_ == nullptr)
        return false;

    instance_ = vulkan_->Instance();
    initialized_ = instance_.device != VK_NULL_HANDLE;

    std::fprintf(
        stderr,
        "[CLOiSimRt] EnsureReady: initialized_=%d "
        "device=%p nativeBackendAvailable=%d\n",
        initialized_,
        reinterpret_cast<void*>(instance_.device),
        VulkanInterceptor::NativeBackendAvailable());

    if (!initialized_)
        return false;

    const bool dispatchLoaded = VulkanDispatch::Instance().Load(
        instance_.instance,
        instance_.device,
        instance_.getInstanceProcAddr);

    std::fprintf(
        stderr,
        "[CLOiSimRt] EnsureReady: dispatchLoaded=%d\n",
        dispatchLoaded);

    if (dispatchLoaded)
    {
        RtRuntimeContext::Instance().Initialize(
            instance_.physicalDevice,
            instance_.device,
            &VulkanDispatch::Instance());
    }

    return dispatchLoaded;
}

bool UnityVulkanBridge::GetCapabilities(CloiSimRtCapabilities* capabilities)
{
    if (capabilities == nullptr)
        return false;

    if (!EnsureReady())
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
