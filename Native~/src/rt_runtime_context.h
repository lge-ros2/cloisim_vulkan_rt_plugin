#pragma once

#include "deferred_release_queue.h"
#include "rt_scene_builder.h"
#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

#include <cstdint>

class RtRuntimeContext
{
public:
    static RtRuntimeContext& Instance();

    bool Initialize(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch);

    void Shutdown();

    void BeginFrame(
        uint64_t currentFrameNumber,
        uint64_t safeFrameNumber);

    void CollectDeferred();

    bool IsInitialized() const { return initialized_; }
    uint64_t CurrentFrameNumber() const { return currentFrameNumber_; }
    uint64_t SafeFrameNumber() const { return safeFrameNumber_; }

    RtSceneBuilder& SceneBuilder() { return sceneBuilder_; }
    DeferredReleaseQueue& DeferredReleases()
    {
        return deferredReleases_;
    }

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
    RtSceneBuilder sceneBuilder_;
    DeferredReleaseQueue deferredReleases_;
    uint64_t currentFrameNumber_ = 0;
    uint64_t safeFrameNumber_ = 0;
    bool initialized_ = false;
};
