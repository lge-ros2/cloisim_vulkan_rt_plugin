#include "rt_runtime_context.h"

#include "lidar_params_gpu.h"

#include <cstring>

RtRuntimeContext& RtRuntimeContext::Instance()
{
    static RtRuntimeContext context;
    return context;
}

bool RtRuntimeContext::Initialize(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch)
{
    Shutdown();

    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        dispatch == nullptr ||
        !dispatch->IsLoaded())
    {
        return false;
    }

    physicalDevice_ = physicalDevice;
    device_ = device;
    dispatch_ = dispatch;
    if (!nativeScene_.Initialize(
            physicalDevice_,
            device_,
            dispatch_))
    {
        Shutdown();
        return false;
    }

    initialized_ = true;
    return true;
}

void RtRuntimeContext::Shutdown()
{
    outputTexture_.DestroyView(device_);
    traceRecorder_.Shutdown();
    nativeScene_.Destroy();
    depthPipeline_.Shutdown();
    lidarTraceRecorder_.Shutdown();
    lidarPipeline_.Shutdown();
    lidarOutputBuffer_.Reset();
    lidarParamsUbo_.Destroy();
    // Graphics device shutdown is the final ownership boundary. Unity has
    // stopped using the device before this callback is issued.
    deferredReleases_.ClearUnsafe();
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    dispatch_ = nullptr;
    currentFrameNumber_ = 0;
    safeFrameNumber_ = 0;
    nativeOutputTexture_ = nullptr;
    outputWidth_ = 0;
    outputHeight_ = 0;
    lastTraceStatus_ = 0;
    lastLidarTraceStatus_ = 0;
    initialized_ = false;
}

void RtRuntimeContext::BeginFrame(
    uint64_t currentFrameNumber,
    uint64_t safeFrameNumber)
{
    if (!initialized_)
        return;

    currentFrameNumber_ = currentFrameNumber;
    safeFrameNumber_ = safeFrameNumber;
    CollectDeferred();
}

void RtRuntimeContext::CollectDeferred()
{
    if (!initialized_)
        return;

    deferredReleases_.Collect(safeFrameNumber_);
}

bool RtRuntimeContext::SetShaderDirectory(const char* path)
{
    if (path == nullptr || path[0] == '\0')
        return false;

    shaderDirectory_ = std::filesystem::path(path);
    return true;
}

bool RtRuntimeContext::InitializeDepthPipeline()
{
    if (!initialized_ ||
        physicalDevice_ == VK_NULL_HANDLE ||
        device_ == VK_NULL_HANDLE ||
        dispatch_ == nullptr ||
        shaderDirectory_.empty())
    {
        return false;
    }

    return depthPipeline_.Initialize(
        physicalDevice_,
        device_,
        dispatch_,
        shaderDirectory_);
}

bool RtRuntimeContext::IsDepthPipelineReady() const
{
    return initialized_ && depthPipeline_.IsReady();
}

bool RtRuntimeContext::SetDepthOutput(
    void* nativeTexture,
    uint32_t width,
    uint32_t height)
{
    if (nativeTexture == nullptr ||
        width == 0 ||
        height == 0)
    {
        return false;
    }

    nativeOutputTexture_ = nativeTexture;
    outputWidth_ = width;
    outputHeight_ = height;
    lastTraceStatus_ = 0;
    return true;
}

bool RtRuntimeContext::UploadMesh(
    uint64_t meshId,
    const float* vertices,
    uint32_t vertexCount,
    const uint32_t* indices,
    uint32_t indexCount)
{
    if (!initialized_)
        return false;

    return nativeScene_.StageMesh(
        meshId, vertices, vertexCount, indices, indexCount);
}

bool RtRuntimeContext::ReleaseMesh(uint64_t meshId)
{
    if (!initialized_)
        return false;

    return nativeScene_.ReleaseMesh(
        meshId, deferredReleases_, currentFrameNumber_);
}

bool RtRuntimeContext::SetSceneInstances(
    const CloiSimRtInstanceDesc* instances,
    uint32_t count)
{
    if (!initialized_)
        return false;

    return nativeScene_.SetInstances(instances, count);
}

bool RtRuntimeContext::RecordSceneBuild(
    VkCommandBuffer commandBuffer)
{
    if (!initialized_ ||
        commandBuffer == VK_NULL_HANDLE ||
        dispatch_ == nullptr)
    {
        return false;
    }

    return nativeScene_.RecordBuild(
        commandBuffer, deferredReleases_, currentFrameNumber_);
}

bool RtRuntimeContext::RecordDepthTrace(
    IUnityGraphicsVulkanV2* vulkan)
{
    lastTraceStatus_ = -1;

    if (!initialized_ ||
        vulkan == nullptr ||
        !depthPipeline_.IsReady() ||
        !nativeScene_.IsReady() ||
        nativeOutputTexture_ == nullptr ||
        outputWidth_ == 0 ||
        outputHeight_ == 0)
    {
        return false;
    }

    outputTexture_.DestroyView(device_);

    if (!outputTexture_.AccessForStorage(
            vulkan,
            nativeOutputTexture_))
    {
        return false;
    }

    if (outputTexture_.Image().format !=
        VK_FORMAT_R32_SFLOAT)
    {
        return false;
    }

    if (!outputTexture_.EnsureView(device_))
        return false;

    // AccessTexture() 호출로 이전 recording state는 무효화되므로 다시
    // 획득한다. frame 번호는 OnRenderEvent에서 이미 BeginFrame()으로
    // 설정되어 있으므로 여기서 다시 호출해 중복 Collect할 필요는 없다.
    UnityVulkanRecordingState recordingState{};

    if (!vulkan->CommandRecordingState(
            &recordingState,
            kUnityVulkanGraphicsQueueAccess_DontCare))
    {
        return false;
    }

    if (!traceRecorder_.Initialize(
            device_,
            &depthPipeline_.Pipeline(),
            &depthPipeline_.Sbt()))
    {
        return false;
    }

    const bool recorded = traceRecorder_.Record(
        recordingState.commandBuffer,
        nativeScene_.Tlas(),
        outputTexture_.View(),
        outputWidth_,
        outputHeight_);

    lastTraceStatus_ = recorded ? 1 : -1;
    return recorded;
}

bool RtRuntimeContext::IsSceneReady() const
{
    return initialized_ && nativeScene_.IsReady();
}

int RtRuntimeContext::LastTraceStatus() const
{
    return lastTraceStatus_;
}

bool RtRuntimeContext::InitializeLidarPipeline()
{
    if (!initialized_ ||
        physicalDevice_ == VK_NULL_HANDLE ||
        device_ == VK_NULL_HANDLE ||
        dispatch_ == nullptr ||
        shaderDirectory_.empty())
    {
        return false;
    }

    return lidarPipeline_.Initialize(
        physicalDevice_,
        device_,
        dispatch_,
        shaderDirectory_);
}

bool RtRuntimeContext::IsLidarPipelineReady() const
{
    return initialized_ && lidarPipeline_.IsReady();
}

bool RtRuntimeContext::RecordLidarTrace(
    IUnityGraphicsVulkanV2* vulkan,
    const CloiSimRtLidarTraceRequest* request)
{
    lastLidarTraceStatus_ = -1;

    if (!initialized_ ||
        vulkan == nullptr ||
        request == nullptr ||
        !lidarPipeline_.IsReady() ||
        !nativeScene_.IsReady() ||
        request->nativeOutputBuffer == nullptr ||
        request->outputElementCount == 0)
    {
        return false;
    }

    if (!lidarOutputBuffer_.AccessForStorage(
            vulkan,
            request->nativeOutputBuffer))
    {
        return false;
    }

    const VkDeviceSize requiredBytes =
        static_cast<VkDeviceSize>(request->outputElementCount) * sizeof(float);
    if (lidarOutputBuffer_.Size() < requiredBytes)
        return false;

    LidarParamsGpu gpuParams{};
    gpuParams.samplesH = request->params.samplesH;
    gpuParams.samplesV = request->params.samplesV;
    gpuParams.angleMinH = request->params.angleMinH;
    gpuParams.angleStepH = request->params.angleStepH;
    gpuParams.angleMinV = request->params.angleMinV;
    gpuParams.angleStepV = request->params.angleStepV;
    gpuParams.rangeMin = request->params.rangeMin;
    gpuParams.rangeMax = request->params.rangeMax;
    gpuParams.rangeLinearResolution = request->params.rangeLinearResolution;
    gpuParams.sensorPositionX = request->params.sensorPosition[0];
    gpuParams.sensorPositionY = request->params.sensorPosition[1];
    gpuParams.sensorPositionZ = request->params.sensorPosition[2];
    gpuParams.sensorRightX = request->params.sensorRight[0];
    gpuParams.sensorRightY = request->params.sensorRight[1];
    gpuParams.sensorRightZ = request->params.sensorRight[2];
    gpuParams.sensorUpX = request->params.sensorUp[0];
    gpuParams.sensorUpY = request->params.sensorUp[1];
    gpuParams.sensorUpZ = request->params.sensorUp[2];
    gpuParams.sensorForwardX = request->params.sensorForward[0];
    gpuParams.sensorForwardY = request->params.sensorForward[1];
    gpuParams.sensorForwardZ = request->params.sensorForward[2];
    gpuParams.selfExclusionId = request->params.selfExclusionId;
    gpuParams.maxSelfHitRetraces = request->params.maxSelfHitRetraces;

    if (!lidarParamsUbo_.IsValid())
    {
        if (!lidarParamsUbo_.Create(
                physicalDevice_,
                device_,
                dispatch_,
                sizeof(LidarParamsGpu),
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        {
            return false;
        }
    }

    if (!lidarParamsUbo_.Upload(&gpuParams, sizeof(gpuParams)))
        return false;

    // AccessBuffer() above invalidates any prior recording state (same
    // caveat as AccessTexture() in RecordDepthTrace) — re-acquire it here.
    UnityVulkanRecordingState recordingState{};
    if (!vulkan->CommandRecordingState(
            &recordingState,
            kUnityVulkanGraphicsQueueAccess_DontCare))
    {
        return false;
    }

    if (!lidarTraceRecorder_.Initialize(
            device_,
            &lidarPipeline_.Pipeline(),
            &lidarPipeline_.Sbt()))
    {
        return false;
    }

    const bool recorded = lidarTraceRecorder_.Record(
        recordingState.commandBuffer,
        nativeScene_.Tlas(),
        lidarOutputBuffer_.Handle(),
        lidarOutputBuffer_.Size(),
        lidarParamsUbo_.Handle(),
        lidarParamsUbo_.Size(),
        request->params.samplesH,
        request->params.samplesV);

    lastLidarTraceStatus_ = recorded ? 1 : -1;
    return recorded;
}

int RtRuntimeContext::LastLidarTraceStatus() const
{
    return lastLidarTraceStatus_;
}
