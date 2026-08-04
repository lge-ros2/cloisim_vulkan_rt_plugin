#pragma once

#include <cstdint>

// Host-side mirror of lidar.rgen's `LidarParams` std140 UBO block — field
// order/padding must match exactly. All groups are 16-byte (vec4/uvec4)
// aligned, so this is unambiguous under std140 (no vec3 packing).
struct LidarParamsGpu
{
    uint32_t samplesH, samplesV, pad0, pad1;
    float angleMinH, angleStepH, angleMinV, angleStepV;
    float rangeMin, rangeMax, rangeLinearResolution, pad2;
    float sensorPositionX, sensorPositionY, sensorPositionZ, pad3;
    float sensorRightX, sensorRightY, sensorRightZ, pad4;
    float sensorUpX, sensorUpY, sensorUpZ, pad5;
    float sensorForwardX, sensorForwardY, sensorForwardZ, pad6;
    uint32_t selfExclusionId, maxSelfHitRetraces, pad7, pad8;
};

static_assert(sizeof(LidarParamsGpu) == 128, "LidarParamsGpu must match lidar.rgen's UBO block size");
