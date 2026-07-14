#include "deferred_release_queue.h"

#include <algorithm>
#include <utility>

void DeferredReleaseQueue::Retire(
    GpuBuffer&& buffer,
    uint64_t lastUsedFrame)
{
    if (!buffer.IsValid())
        return;

    buffers_.push_back(BufferEntry{
        lastUsedFrame,
        std::move(buffer)});
}

void DeferredReleaseQueue::Retire(
    AccelerationStructure&& accelerationStructure,
    uint64_t lastUsedFrame)
{
    if (!accelerationStructure.IsValid())
        return;

    accelerationStructures_.push_back(AccelerationStructureEntry{
        lastUsedFrame,
        std::move(accelerationStructure)});
}

void DeferredReleaseQueue::Collect(uint64_t safeFrameNumber)
{
    accelerationStructures_.erase(
        std::remove_if(
            accelerationStructures_.begin(),
            accelerationStructures_.end(),
            [safeFrameNumber](const AccelerationStructureEntry& entry)
            {
                return entry.lastUsedFrame <= safeFrameNumber;
            }),
        accelerationStructures_.end());

    buffers_.erase(
        std::remove_if(
            buffers_.begin(),
            buffers_.end(),
            [safeFrameNumber](const BufferEntry& entry)
            {
                return entry.lastUsedFrame <= safeFrameNumber;
            }),
        buffers_.end());
}

void DeferredReleaseQueue::ClearUnsafe()
{
    accelerationStructures_.clear();
    buffers_.clear();
}

std::size_t DeferredReleaseQueue::PendingBufferCount() const
{
    return buffers_.size();
}

std::size_t DeferredReleaseQueue::PendingAccelerationStructureCount() const
{
    return accelerationStructures_.size();
}
