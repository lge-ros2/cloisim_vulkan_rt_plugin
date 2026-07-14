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

static void UNITY_INTERFACE_API OnRenderEvent(int eventId, void*)
{
    auto* vulkan = UnityVulkanBridge::Instance().Vulkan();
    if (vulkan == nullptr)
        return;

    if (eventId < 1 || eventId > 4)
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
        context.RecordSmokeBuild(state.commandBuffer);
        break;

    case 3:
        // RecordDepthTrace() 내부 AccessTexture()가 state를 무효화하므로
        // 함수 안에서 CommandRecordingState()를 다시 획득한다.
        context.RecordDepthTrace(vulkan);
        break;

    case 4:
        context.CollectDeferred();
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
        OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
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
    return 2;
}

int32_t CLOISimRt_GetCapabilities(CloiSimRtCapabilities* capabilities)
{
    return UnityVulkanBridge::Instance().GetCapabilities(capabilities) ? 0 : -1;
}

int32_t CLOISimRt_IsNativeBackendAvailable()
{
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

int32_t CLOISimRt_IsSmokeSceneReady()
{
    return RtRuntimeContext::Instance().IsSmokeSceneReady()
        ? 1
        : 0;
}

int32_t CLOISimRt_GetLastTraceStatus()
{
    return RtRuntimeContext::Instance().LastTraceStatus();
}
