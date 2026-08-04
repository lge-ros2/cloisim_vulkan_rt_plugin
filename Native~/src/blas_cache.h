#pragma once

#include "acceleration_structure.h"
#include "deferred_release_queue.h"
#include "gpu_buffer.h"
#include "rt_scene_builder.h"
#include "vulkan_dispatch.h"

#include <cstdint>
#include <unordered_map>
#include <vulkan/vulkan.h>

// Per-mesh BLAS cache keyed by an opaque meshId (the caller — the C# side —
// packs (Mesh.GetInstanceID(), subMeshIndex) into it). A mesh's geometry is
// staged once via StageMesh() and its BLAS is built at most once via
// EnsureBuilt(); as long as the same meshId is referenced by later instances,
// the cached BLAS is reused without rebuilding, mirroring how Unity's own
// UnifiedRayTracing package already treats per-mesh BLAS caching as "free".
class BlasCache
{
public:
    bool StageMesh(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch,
        uint64_t meshId,
        const float* vertices,
        uint32_t vertexCount,
        const uint32_t* indices,
        uint32_t indexCount);

    // Retires this mesh's GPU resources into the deferred queue (a trace
    // against its BLAS from a previous frame may still be in flight) and
    // drops it from the cache.
    void ReleaseMesh(
        uint64_t meshId,
        DeferredReleaseQueue& queue,
        uint64_t lastUsedFrame);

    // Builds meshId's BLAS if it was staged but not yet built. No-op
    // (returns true) if already built. Returns false if meshId was never
    // staged or the build failed.
    bool EnsureBuilt(
        VkCommandBuffer commandBuffer,
        uint64_t meshId,
        RtSceneBuilder& builder,
        DeferredReleaseQueue& queue,
        uint64_t currentFrame);

    bool HasBlas(uint64_t meshId) const;
    VkDeviceAddress BlasDeviceAddress(uint64_t meshId) const;

    // Immediate teardown of every cached mesh. Only safe to call once the
    // GPU is known idle (the same "final ownership boundary" contract as
    // RtRuntimeContext::Shutdown) — not fence-gated.
    void Clear();

private:
    struct Entry
    {
        GpuBuffer vertexBuffer;
        GpuBuffer indexBuffer;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        AccelerationStructure blas;
    };

    std::unordered_map<uint64_t, Entry> entries_;
};
