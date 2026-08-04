#include "native_scene.h"

#include <cstring>
#include <unordered_set>
#include <utility>

bool NativeScene::Initialize(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch)
{
    Destroy();

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
    return builder_.Initialize(physicalDevice, device, dispatch);
}

bool NativeScene::StageMesh(
    uint64_t meshId,
    const float* vertices,
    uint32_t vertexCount,
    const uint32_t* indices,
    uint32_t indexCount)
{
    return blasCache_.StageMesh(
        physicalDevice_, device_, dispatch_,
        meshId, vertices, vertexCount, indices, indexCount);
}

bool NativeScene::ReleaseMesh(
    uint64_t meshId,
    DeferredReleaseQueue& queue,
    uint64_t currentFrame)
{
    blasCache_.ReleaseMesh(meshId, queue, currentFrame);
    return true;
}

bool NativeScene::SetInstances(
    const CloiSimRtInstanceDesc* instances,
    uint32_t count)
{
    if (instances == nullptr && count > 0)
        return false;

    pendingInstances_.assign(instances, instances + count);
    return true;
}

bool NativeScene::RecordBuild(
    VkCommandBuffer commandBuffer,
    DeferredReleaseQueue& deferred,
    uint64_t currentFrame)
{
    ready_ = false;

    if (commandBuffer == VK_NULL_HANDLE || dispatch_ == nullptr)
        return false;

    // Build any BLASes referenced by this instance list that were staged
    // but not yet built. Already-built meshes are a cached no-op — this is
    // the whole point of BlasCache (avoid rebuilding a BLAS every frame for
    // every instance that shares a mesh with an already-built one).
    bool builtAnyBlas = false;
    {
        std::unordered_set<uint64_t> processed;
        for (const auto& desc : pendingInstances_)
        {
            if (!processed.insert(desc.meshId).second)
                continue;

            if (blasCache_.HasBlas(desc.meshId))
                continue;

            if (blasCache_.EnsureBuilt(
                    commandBuffer, desc.meshId, builder_, deferred, currentFrame))
            {
                builtAnyBlas = true;
            }
        }
    }

    if (builtAnyBlas)
    {
        // A TLAS build reads each instance's BLAS via its device address;
        // make sure any BLAS build just recorded above has actually
        // completed before that read, mirroring the barrier the old
        // SmokeScene inserted between its BLAS and TLAS builds.
        VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
        barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
        barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
            0, 1, &barrier, 0, nullptr, 0, nullptr);
    }

    // A KHR TLAS build always produces a new AS handle — the previous one
    // (if any) must be deferred-freed, not destroyed in place, since a
    // trace against it from a prior frame may still be in flight.
    deferred.Retire(std::move(tlas_), currentFrame);
    deferred.Retire(std::move(tlasScratch_), currentFrame);
    deferred.Retire(tlasInstanceBuffer_.ReleaseBuffer(), currentFrame);

    if (pendingInstances_.empty())
    {
        ready_ = true; // empty scene is valid — every ray simply misses
        return true;
    }

    std::vector<VkAccelerationStructureInstanceKHR> instances;
    instances.reserve(pendingInstances_.size());
    for (const auto& desc : pendingInstances_)
    {
        const auto blasAddress = blasCache_.BlasDeviceAddress(desc.meshId);
        if (blasAddress == 0)
            continue; // defensive: mesh not uploaded/built yet — should not happen

        VkAccelerationStructureInstanceKHR instance{};
        std::memcpy(&instance.transform, desc.transform, sizeof(instance.transform));
        instance.instanceCustomIndex = desc.instanceId;
        instance.mask = desc.mask;
        instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        instance.accelerationStructureReference = blasAddress;
        instances.push_back(instance);
    }

    if (instances.empty())
    {
        ready_ = true;
        return true;
    }

    if (!tlasInstanceBuffer_.CreateAndUpload(
            physicalDevice_, device_, dispatch_, instances))
    {
        return false;
    }

    if (!builder_.CreateTlas(
            commandBuffer,
            tlasInstanceBuffer_.DeviceAddress(),
            tlasInstanceBuffer_.InstanceCount(),
            tlas_,
            tlasScratch_))
    {
        return false;
    }

    RtSceneBuilder::InsertBuildBarrier(commandBuffer);
    ready_ = true;
    return true;
}

void NativeScene::Destroy()
{
    ready_ = false;
    tlas_.Destroy();
    tlasScratch_.Destroy();
    tlasInstanceBuffer_.Destroy();
    blasCache_.Clear();
    pendingInstances_.clear();
    physicalDevice_ = VK_NULL_HANDLE;
    device_ = VK_NULL_HANDLE;
    dispatch_ = nullptr;
}
