#include "blas_cache.h"

bool BlasCache::StageMesh(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch,
    uint64_t meshId,
    const float* vertices,
    uint32_t vertexCount,
    const uint32_t* indices,
    uint32_t indexCount)
{
    // Meshes referenced by native-backend instances are static (skinned
    // meshes are out of scope for v1 — see NativeScene's doc comment), so a
    // meshId's geometry never changes after its first upload. Treat a
    // repeat StageMesh() for an already-known meshId as an idempotent no-op
    // rather than re-uploading.
    if (entries_.find(meshId) != entries_.end())
        return true;

    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        dispatch == nullptr ||
        !dispatch->IsLoaded() ||
        vertices == nullptr ||
        vertexCount == 0 ||
        indices == nullptr ||
        indexCount < 3)
    {
        return false;
    }

    Entry entry{};
    entry.vertexCount = vertexCount;
    entry.indexCount = indexCount;

    const auto usage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    const VkDeviceSize vertexBytes =
        static_cast<VkDeviceSize>(vertexCount) * 3 * sizeof(float);
    const VkDeviceSize indexBytes =
        static_cast<VkDeviceSize>(indexCount) * sizeof(uint32_t);

    if (!entry.vertexBuffer.Create(
            physicalDevice, device, dispatch, vertexBytes, usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !entry.vertexBuffer.Upload(vertices, vertexBytes) ||
        !entry.indexBuffer.Create(
            physicalDevice, device, dispatch, indexBytes, usage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !entry.indexBuffer.Upload(indices, indexBytes))
    {
        return false;
    }

    entries_.emplace(meshId, std::move(entry));
    return true;
}

void BlasCache::ReleaseMesh(
    uint64_t meshId,
    DeferredReleaseQueue& queue,
    uint64_t lastUsedFrame)
{
    auto it = entries_.find(meshId);
    if (it == entries_.end())
        return;

    queue.Retire(std::move(it->second.blas), lastUsedFrame);
    queue.Retire(std::move(it->second.vertexBuffer), lastUsedFrame);
    queue.Retire(std::move(it->second.indexBuffer), lastUsedFrame);
    entries_.erase(it);
}

bool BlasCache::EnsureBuilt(
    VkCommandBuffer commandBuffer,
    uint64_t meshId,
    RtSceneBuilder& builder,
    DeferredReleaseQueue& queue,
    uint64_t currentFrame)
{
    auto it = entries_.find(meshId);
    if (it == entries_.end())
        return false;

    auto& entry = it->second;
    if (entry.blas.IsValid())
        return true; // already built — cached

    BlasTriangleInput input{};
    input.vertexAddress = entry.vertexBuffer.DeviceAddress();
    input.indexAddress = entry.indexBuffer.DeviceAddress();
    input.vertexCount = entry.vertexCount;
    input.vertexStride = 3 * sizeof(float);
    input.indexCount = entry.indexCount;

    GpuBuffer scratch;
    if (!builder.CreateBlas(commandBuffer, input, entry.blas, scratch))
        return false;

    // The scratch buffer is only needed while the BLAS build executes on the
    // GPU; retire it through the deferred queue rather than freeing
    // immediately, matching the existing SmokeScene/TLAS scratch pattern.
    queue.Retire(std::move(scratch), currentFrame);
    return true;
}

bool BlasCache::HasBlas(uint64_t meshId) const
{
    auto it = entries_.find(meshId);
    return it != entries_.end() && it->second.blas.IsValid();
}

VkDeviceAddress BlasCache::BlasDeviceAddress(uint64_t meshId) const
{
    auto it = entries_.find(meshId);
    if (it == entries_.end())
        return 0;
    return it->second.blas.DeviceAddress();
}

void BlasCache::Clear()
{
    entries_.clear();
}
