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
    bool GetCapabilities(CloiSimRtCapabilities* capabilities) const;
    IUnityGraphicsVulkanV2* Vulkan() const;

private:
    IUnityGraphicsVulkanV2* vulkan_ = nullptr;
    UnityVulkanInstance instance_{};
    bool initialized_ = false;
};
