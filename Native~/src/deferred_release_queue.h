#pragma once

#include "acceleration_structure.h"
#include "gpu_buffer.h"

#include <cstdint>
#include <vector>

class DeferredReleaseQueue
{
public:
    void Retire(GpuBuffer&& buffer, uint64_t lastUsedFrame);
    void Retire(
        AccelerationStructure&& accelerationStructure,
        uint64_t lastUsedFrame);

    void Collect(uint64_t safeFrameNumber);
    void ClearUnsafe();

    std::size_t PendingBufferCount() const;
    std::size_t PendingAccelerationStructureCount() const;

private:
    struct BufferEntry
    {
        uint64_t lastUsedFrame = 0;
        GpuBuffer resource;
    };

    struct AccelerationStructureEntry
    {
        uint64_t lastUsedFrame = 0;
        AccelerationStructure resource;
    };

    std::vector<BufferEntry> buffers_;
    std::vector<AccelerationStructureEntry> accelerationStructures_;
};
