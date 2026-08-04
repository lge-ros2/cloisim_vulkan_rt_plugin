#pragma once

#include "blas_cache.h"
#include "cloisim_vulkan_rt/api.h"
#include "deferred_release_queue.h"
#include "gpu_buffer.h"
#include "rt_scene_builder.h"
#include "tlas_instance_buffer.h"
#include "vulkan_dispatch.h"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

// Single shared scene built from real application geometry, replacing the
// former hard-coded one-triangle SmokeScene. Persists across frames (no
// full teardown+rebuild every call): a mesh's BLAS is cached and reused
// across instances/frames (see BlasCache), while the TLAS is rebuilt
// whenever the caller's instance list actually changed (the caller — the
// C# side — is responsible for only calling SetInstances()+RecordBuild()
// when something real changed, mirroring the existing Compute backend's
// skip-rebuild-when-unchanged optimization).
//
// v1 scope: static geometry only (no skinned/animated mesh support — the
// caller is expected to not reference skinned renderers here at all).
class NativeScene
{
public:
    bool Initialize(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch);

    // Immediate (no command buffer): uploads/caches meshId's geometry.
    // Idempotent if meshId was already staged.
    bool StageMesh(
        uint64_t meshId,
        const float* vertices,
        uint32_t vertexCount,
        const uint32_t* indices,
        uint32_t indexCount);

    // Immediate: retires meshId's cached BLAS/buffers via the deferred
    // queue and drops it from the cache.
    bool ReleaseMesh(
        uint64_t meshId,
        DeferredReleaseQueue& queue,
        uint64_t currentFrame);

    // Immediate: replaces the pending instance list consumed by the next
    // RecordBuild() call.
    bool SetInstances(
        const CloiSimRtInstanceDesc* instances,
        uint32_t count);

    // Builds any not-yet-built BLASes referenced by the pending instance
    // list, then rebuilds the TLAS from that list. The previous TLAS (if
    // any) is retired via the deferred queue, never destroyed in place.
    bool RecordBuild(
        VkCommandBuffer commandBuffer,
        DeferredReleaseQueue& deferred,
        uint64_t currentFrame);

    // Immediate teardown of every owned GPU resource. Only safe once the
    // GPU is known idle (same contract as RtRuntimeContext::Shutdown).
    void Destroy();

    bool IsReady() const { return ready_; }
    VkAccelerationStructureKHR Tlas() const { return tlas_.Handle(); }

private:
    BlasCache blasCache_;
    RtSceneBuilder builder_;
    std::vector<CloiSimRtInstanceDesc> pendingInstances_;
    TlasInstanceBuffer tlasInstanceBuffer_;
    AccelerationStructure tlas_;
    GpuBuffer tlasScratch_;
    bool ready_ = false;

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
};
