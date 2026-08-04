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

// Real-scene geometry upload/instancing, replacing the former hard-coded
// one-triangle smoke-test scene. meshId is an opaque caller-assigned key
// (the C# side packs Mesh.GetInstanceID()/subMeshIndex into it) used to
// cache a mesh's BLAS across instances/frames — see NativeScene/BlasCache.
struct CloiSimRtMeshDesc
{
    uint64_t meshId;
    const float* vertices;      // xyz packed, length = vertexCount * 3
    uint32_t vertexCount;
    const uint32_t* indices;    // triangle list, length = indexCount (multiple of 3)
    uint32_t indexCount;
};

// Copies vertex/index data synchronously before returning; the caller only
// needs to keep the pointers valid for the duration of the call. Idempotent
// for an already-uploaded meshId (mesh geometry is assumed immutable once
// uploaded — no skinned/animated mesh support in this ABI).
CLOISIM_RT_EXPORT int32_t
CLOISimRt_UploadMesh(const CloiSimRtMeshDesc* desc);

// Retires meshId's cached BLAS/buffers once any in-flight trace against it
// has completed, and drops it from the cache. Safe to call even if meshId
// is still referenced by SetSceneInstances()'s last snapshot; that snapshot
// should be updated to drop the instance first.
CLOISIM_RT_EXPORT int32_t
CLOISimRt_ReleaseMesh(uint64_t meshId);

struct CloiSimRtInstanceDesc
{
    uint64_t meshId;
    float transform[12];   // row-major 3x4, matches VkTransformMatrixKHR
    uint32_t instanceId;   // -> instanceCustomIndex (24-bit HW limit)
    uint32_t mask;          // typically 0xFF
};

// Replaces the pending instance list consumed by the next BuildScene
// (eventId=2) render event. Every meshId referenced here must already have
// been uploaded via CLOISimRt_UploadMesh in the same or an earlier call.
CLOISIM_RT_EXPORT int32_t
CLOISimRt_SetSceneInstances(const CloiSimRtInstanceDesc* instances, uint32_t count);

CLOISIM_RT_EXPORT int32_t
CLOISimRt_IsSceneReady();

CLOISIM_RT_EXPORT int32_t
CLOISimRt_GetLastTraceStatus();

// Lidar ray-trace pipeline (lidar.rgen/rmiss/rchit), a peer to the depth
// pipeline above — reads the same shared scene (see CLOISimRt_SetSceneInstances)
// but casts an arbitrary spherical pattern of rays instead of one ray per
// screen pixel, writing distances into a caller-provided buffer instead of
// a texture. See Assets/Resources/Shader/LidarRayTrace.compute (cloisim
// repo) for the parameter semantics this reproduces.
CLOISIM_RT_EXPORT int32_t
CLOISimRt_InitializeLidarPipeline();

CLOISIM_RT_EXPORT int32_t
CLOISimRt_IsLidarPipelineReady();

struct CloiSimRtLidarParams
{
    uint32_t samplesH, samplesV;
    float angleMinH, angleStepH, angleMinV, angleStepV;
    float rangeMin, rangeMax, rangeLinearResolution;
    float sensorPosition[3], sensorRight[3], sensorUp[3], sensorForward[3];
    uint32_t selfExclusionId;
    uint32_t maxSelfHitRetraces; // convention: 8, matches LidarRayTrace.compute
};

struct CloiSimRtLidarTraceRequest
{
    void* nativeOutputBuffer;      // GraphicsBuffer/ComputeBuffer native pointer
    uint32_t outputElementCount;   // must equal samplesH*samplesV
    uint32_t reserved;
    CloiSimRtLidarParams params;
};

CLOISIM_RT_EXPORT int32_t
CLOISimRt_GetLastLidarTraceStatus();
