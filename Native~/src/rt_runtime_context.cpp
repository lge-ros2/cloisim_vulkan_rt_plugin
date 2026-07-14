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

    if (!sceneBuilder_.Initialize(
            physicalDevice,
            device,
            dispatch))
    {
        return false;
    }

    physicalDevice_ = physicalDevice;
    device_ = device;
    dispatch_ = dispatch;
    initialized_ = true;
    return true;
}

void RtRuntimeContext::Shutdown()
{
    depthPipeline_.Shutdown();
    // Graphics device shutdown is the final ownership boundary. Unity has
    // stopped using the device before this callback is issued.
    deferredReleases_.ClearUnsafe();
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    dispatch_ = nullptr;
    currentFrameNumber_ = 0;
    safeFrameNumber_ = 0;
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
