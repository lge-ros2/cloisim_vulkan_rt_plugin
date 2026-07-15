#include "rt_runtime_context.h"

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
    if (!smokeScene_.Initialize(
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
    smokeScene_.Destroy();
    depthPipeline_.Shutdown();
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

bool RtRuntimeContext::RecordSmokeBuild(
    VkCommandBuffer commandBuffer)
{
    if (!initialized_ ||
        commandBuffer == VK_NULL_HANDLE ||
        dispatch_ == nullptr)
    {
        return false;
    }

    smokeScene_.RetireInto(deferredReleases_, currentFrameNumber_);

    if (!smokeScene_.Initialize(
            physicalDevice_,
            device_,
            dispatch_))
    {
        return false;
    }

    if (!smokeScene_.RecordBuild(commandBuffer))
        return false;

    deferredReleases_.Retire(
        smokeScene_.ReleaseBlasScratch(),
        currentFrameNumber_);

    deferredReleases_.Retire(
        smokeScene_.ReleaseTlasScratch(),
        currentFrameNumber_);

    return true;
}

bool RtRuntimeContext::RecordDepthTrace(
    IUnityGraphicsVulkanV2* vulkan)
{
    lastTraceStatus_ = -1;

    if (!initialized_ ||
        vulkan == nullptr ||
        !depthPipeline_.IsReady() ||
        !smokeScene_.IsReady() ||
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
        smokeScene_.Tlas(),
        outputTexture_.View(),
        outputWidth_,
        outputHeight_);

    lastTraceStatus_ = recorded ? 1 : -1;
    return recorded;
}

bool RtRuntimeContext::IsSmokeSceneReady() const
{
    return initialized_ && smokeScene_.IsReady();
}

int RtRuntimeContext::LastTraceStatus() const
{
    return lastTraceStatus_;
}
