#pragma once

#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"
#include "cloisim_vulkan_rt/api.h"

class UnityVulkanBridge
{
public:
    static UnityVulkanBridge& Instance();

    void Load(IUnityInterfaces* interfaces);
    bool InstallInitializationInterceptor();
    void OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType);
    bool GetCapabilities(CloiSimRtCapabilities* capabilities);
    IUnityGraphicsVulkanV2* Vulkan() const;

    // Lazily queries UnityVulkanInstance and initializes the RT runtime.
    // Must NOT be called synchronously from within the
    // kUnityGfxDeviceEventInitialize callback when a device-creation
    // interceptor (vkCreateDevice hook) is installed: at that point Unity
    // is still inside its own device bring-up call stack and its
    // UnityVulkanInstance bookkeeping is not yet fully populated, so
    // querying vulkan_->Instance() there crashes. Safe to call from any
    // later point (managed-code entry points, render events), since those
    // only run after engine startup has fully completed.
    bool EnsureReady();

private:
    IUnityGraphicsVulkanV2* vulkan_ = nullptr;
    UnityVulkanInstance instance_{};
    bool initialized_ = false;
    bool deviceEventSeen_ = false;
};
