#include "IUnityInterface.h"
#include "IUnityGraphics.h"
#include "IUnityGraphicsVulkan.h"

#include "cloisim_vulkan_rt/api.h"
#include "unity_vulkan_bridge.h"
#include "vulkan_interceptor.h"
#include "rt_runtime_context.h"

static IUnityGraphics* g_graphics = nullptr;

static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(
    UnityGfxDeviceEventType eventType)
{
    UnityVulkanBridge::Instance().OnGraphicsDeviceEvent(eventType);
}

static void UNITY_INTERFACE_API OnRenderEvent(int eventId, void* data)
{
    if (!UnityVulkanBridge::Instance().EnsureReady())
        return;

    auto* vulkan = UnityVulkanBridge::Instance().Vulkan();
    if (vulkan == nullptr)
        return;

    if (eventId < 1 || eventId > 5)
        return;

    vulkan->EnsureOutsideRenderPass();

    UnityVulkanRecordingState state{};
    if (!vulkan->CommandRecordingState(
            &state,
            kUnityVulkanGraphicsQueueAccess_DontCare))
    {
        return;
    }

    auto& context = RtRuntimeContext::Instance();
    context.BeginFrame(
        state.currentFrameNumber,
        state.safeFrameNumber);

    switch (eventId)
    {
    case 2:
        context.RecordSceneBuild(state.commandBuffer);
        break;

    case 3:
        // RecordDepthTrace() 내부 AccessTexture()가 state를 무효화하므로
        // 함수 안에서 CommandRecordingState()를 다시 획득한다.
        context.RecordDepthTrace(vulkan);
        break;

    case 4:
        context.CollectDeferred();
        break;

    case 5:
        // RecordLidarTrace() 내부 AccessBuffer()가 state를 무효화하므로
        // 함수 안에서 CommandRecordingState()를 다시 획득한다.
        context.RecordLidarTrace(
            vulkan,
            static_cast<const CloiSimRtLidarTraceRequest*>(data));
        break;

    default:
        break;
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* interfaces)
{
    g_graphics = interfaces->Get<IUnityGraphics>();
    UnityVulkanBridge::Instance().Load(interfaces);
    UnityVulkanBridge::Instance().InstallInitializationInterceptor();

    if (g_graphics != nullptr)
    {
        g_graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    if (g_graphics != nullptr)
        g_graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);

    OnGraphicsDeviceEvent(kUnityGfxDeviceEventShutdown);
    g_graphics = nullptr;
}

uint32_t CLOISimRt_GetAbiVersion()
{
    return 3;
}

int32_t CLOISimRt_GetCapabilities(CloiSimRtCapabilities* capabilities)
{
    return UnityVulkanBridge::Instance().GetCapabilities(capabilities) ? 0 : -1;
}

int32_t CLOISimRt_IsNativeBackendAvailable()
{
    UnityVulkanBridge::Instance().EnsureReady();
    return VulkanInterceptor::NativeBackendAvailable() ? 1 : 0;
}

void* CLOISimRt_GetRenderEventFunc()
{
    return reinterpret_cast<void*>(OnRenderEvent);
}

int32_t CLOISimRt_SetShaderDirectory(const char* path)
{
    return RtRuntimeContext::Instance().SetShaderDirectory(path)
        ? 0
        : -1;
}

int32_t CLOISimRt_InitializeDepthPipeline()
{
    if (!UnityVulkanBridge::Instance().EnsureReady())
        return -1;

    return RtRuntimeContext::Instance().InitializeDepthPipeline()
        ? 0
        : -1;
}

int32_t CLOISimRt_IsDepthPipelineReady()
{
    return RtRuntimeContext::Instance().IsDepthPipelineReady()
        ? 1
        : 0;
}

int32_t CLOISimRt_SetDepthOutput(
    const CloiSimRtDepthOutput* output)
{
    if (output == nullptr)
        return -1;

    return RtRuntimeContext::Instance().SetDepthOutput(
        output->nativeTexture,
        output->width,
        output->height)
        ? 0
        : -1;
}

int32_t CLOISimRt_UploadMesh(const CloiSimRtMeshDesc* desc)
{
    if (desc == nullptr)
        return -1;

    return RtRuntimeContext::Instance().UploadMesh(
        desc->meshId,
        desc->vertices,
        desc->vertexCount,
        desc->indices,
        desc->indexCount)
        ? 0
        : -1;
}

int32_t CLOISimRt_ReleaseMesh(uint64_t meshId)
{
    return RtRuntimeContext::Instance().ReleaseMesh(meshId)
        ? 0
        : -1;
}

int32_t CLOISimRt_SetSceneInstances(
    const CloiSimRtInstanceDesc* instances,
    uint32_t count)
{
    return RtRuntimeContext::Instance().SetSceneInstances(instances, count)
        ? 0
        : -1;
}

int32_t CLOISimRt_IsSceneReady()
{
    return RtRuntimeContext::Instance().IsSceneReady()
        ? 1
        : 0;
}

int32_t CLOISimRt_GetLastTraceStatus()
{
    return RtRuntimeContext::Instance().LastTraceStatus();
}

int32_t CLOISimRt_InitializeLidarPipeline()
{
    if (!UnityVulkanBridge::Instance().EnsureReady())
        return -1;

    return RtRuntimeContext::Instance().InitializeLidarPipeline()
        ? 0
        : -1;
}

int32_t CLOISimRt_IsLidarPipelineReady()
{
    return RtRuntimeContext::Instance().IsLidarPipelineReady()
        ? 1
        : 0;
}

int32_t CLOISimRt_GetLastLidarTraceStatus()
{
    return RtRuntimeContext::Instance().LastLidarTraceStatus();
}
