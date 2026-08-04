#pragma once

#include "native_scene.h"
#include "unity_output_buffer.h"
#include "unity_output_texture.h"
#include "depth_trace_recorder.h"
#include "lidar_trace_recorder.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"

#include "depth_pipeline_resources.h"
#include "lidar_pipeline_resources.h"
#include "deferred_release_queue.h"
#include "gpu_buffer.h"
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

    // Real-scene geometry upload/instancing — see NativeScene.
    bool UploadMesh(
        uint64_t meshId,
        const float* vertices,
        uint32_t vertexCount,
        const uint32_t* indices,
        uint32_t indexCount);
    bool ReleaseMesh(uint64_t meshId);
    bool SetSceneInstances(
        const CloiSimRtInstanceDesc* instances,
        uint32_t count);

    bool RecordSceneBuild(
        VkCommandBuffer commandBuffer);

    bool RecordDepthTrace(
        IUnityGraphicsVulkanV2* vulkan);

    bool IsSceneReady() const;
    int LastTraceStatus() const;

    bool SetShaderDirectory(const char* path);
    bool InitializeDepthPipeline();
    bool IsDepthPipelineReady() const;

    bool InitializeLidarPipeline();
    bool IsLidarPipelineReady() const;
    bool RecordLidarTrace(
        IUnityGraphicsVulkanV2* vulkan,
        const CloiSimRtLidarTraceRequest* request);
    int LastLidarTraceStatus() const;

    bool IsInitialized() const { return initialized_; }
    uint64_t CurrentFrameNumber() const { return currentFrameNumber_; }
    uint64_t SafeFrameNumber() const { return safeFrameNumber_; }
    const DepthPipelineResources& DepthPipeline() const { return depthPipeline_; }
    DeferredReleaseQueue& DeferredReleases()
    {
        return deferredReleases_;
    }

private:
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
    NativeScene nativeScene_;
    UnityOutputTexture outputTexture_;
    DepthTraceRecorder traceRecorder_;

    void* nativeOutputTexture_ = nullptr;
    uint32_t outputWidth_ = 0;
    uint32_t outputHeight_ = 0;
    int lastTraceStatus_ = 0;
    DepthPipelineResources depthPipeline_;

    LidarPipelineResources lidarPipeline_;
    UnityOutputBuffer lidarOutputBuffer_;
    LidarTraceRecorder lidarTraceRecorder_;
    GpuBuffer lidarParamsUbo_;
    int lastLidarTraceStatus_ = 0;

    std::filesystem::path shaderDirectory_;
    DeferredReleaseQueue deferredReleases_;
    uint64_t currentFrameNumber_ = 0;
    uint64_t safeFrameNumber_ = 0;
    bool initialized_ = false;
};
