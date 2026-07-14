#pragma once

#include <cstdint>

#if defined(_WIN32)
#define CLOISIM_RT_EXPORT extern "C" __declspec(dllexport)
#else
#define CLOISIM_RT_EXPORT extern "C" __attribute__((visibility("default")))
#endif

struct CloiSimRtCapabilities
{
    uint32_t abiVersion;
    uint32_t apiVersion;
    uint32_t vendorId;
    uint32_t deviceId;
    uint32_t pluginLoaded;
    uint32_t vulkanDeviceReady;
    uint32_t dispatchLoaded;
    uint32_t accelerationStructure;
    uint32_t rayTracingPipeline;
    uint32_t rayQuery;
    uint32_t bufferDeviceAddress;
    uint32_t maxRecursionDepth;
    uint32_t shaderGroupHandleSize;
    uint32_t shaderGroupBaseAlignment;
};

CLOISIM_RT_EXPORT uint32_t CLOISimRt_GetAbiVersion();
CLOISIM_RT_EXPORT int32_t CLOISimRt_GetCapabilities(CloiSimRtCapabilities* capabilities);
CLOISIM_RT_EXPORT int32_t CLOISimRt_IsNativeBackendAvailable();
CLOISIM_RT_EXPORT void* CLOISimRt_GetRenderEventFunc();

CLOISIM_RT_EXPORT int32_t CLOISimRt_SetShaderDirectory(const char* path);
CLOISIM_RT_EXPORT int32_t CLOISimRt_InitializeDepthPipeline();
CLOISIM_RT_EXPORT int32_t CLOISimRt_IsDepthPipelineReady();

struct CloiSimRtDepthOutput
{
    void* nativeTexture;
    uint32_t width;
    uint32_t height;
    uint32_t reserved;
};

CLOISIM_RT_EXPORT int32_t
CLOISimRt_SetDepthOutput(
    const CloiSimRtDepthOutput* output);

CLOISIM_RT_EXPORT int32_t
CLOISimRt_IsSmokeSceneReady();

CLOISIM_RT_EXPORT int32_t
CLOISimRt_GetLastTraceStatus();
