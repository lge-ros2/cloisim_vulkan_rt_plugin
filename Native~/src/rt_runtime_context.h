#include "smoke_scene.h"
#include "unity_output_texture.h"
#include "depth_trace_recorder.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"
#pragma once

#include "depth_pipeline_resources.h"
#include "deferred_release_queue.h"
#include "rt_scene_builder.h"
#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <filesystem>

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

    bool SetDepthOutput(
        void* nativeTexture,
        uint32_t width,
        uint32_t height);

    bool RecordSmokeBuild(
        VkCommandBuffer commandBuffer);

    bool RecordDepthTrace(
        IUnityGraphicsVulkanV2* vulkan);

    bool IsSmokeSceneReady() const;
    int LastTraceStatus() const;

    bool SetShaderDirectory(const char* path);
    bool InitializeDepthPipeline();
    bool IsDepthPipelineReady() const;

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
    SmokeScene smokeScene_;
    UnityOutputTexture outputTexture_;
    DepthTraceRecorder traceRecorder_;

    void* nativeOutputTexture_ = nullptr;
    uint32_t outputWidth_ = 0;
    uint32_t outputHeight_ = 0;
    int lastTraceStatus_ = 0;
    DepthPipelineResources depthPipeline_;
    std::filesystem::path shaderDirectory_;
    DeferredReleaseQueue deferredReleases_;
    uint64_t currentFrameNumber_ = 0;
    uint64_t safeFrameNumber_ = 0;
    bool initialized_ = false;
};
