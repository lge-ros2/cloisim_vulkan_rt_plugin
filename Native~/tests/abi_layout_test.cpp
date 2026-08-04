#include "cloisim_vulkan_rt/api.h"

#include <cstddef>
#include <cstdint>
#include <iostream>

namespace
{
int Fail(const char* message)
{
    std::cerr << "[FAIL] " << message << '\n';
    return 1;
}
}

int main()
{
    static_assert(sizeof(uint32_t) == 4);
    static_assert(alignof(CloiSimRtCapabilities) == alignof(uint32_t));
    static_assert(sizeof(CloiSimRtCapabilities) == 14 * sizeof(uint32_t));

#define CHECK_OFFSET(field, index)                                      \
    if (offsetof(CloiSimRtCapabilities, field) !=                       \
        (index) * sizeof(uint32_t))                                     \
        return Fail("unexpected offset: " #field)

    CHECK_OFFSET(abiVersion, 0);
    CHECK_OFFSET(apiVersion, 1);
    CHECK_OFFSET(vendorId, 2);
    CHECK_OFFSET(deviceId, 3);
    CHECK_OFFSET(pluginLoaded, 4);
    CHECK_OFFSET(vulkanDeviceReady, 5);
    CHECK_OFFSET(dispatchLoaded, 6);
    CHECK_OFFSET(accelerationStructure, 7);
    CHECK_OFFSET(rayTracingPipeline, 8);
    CHECK_OFFSET(rayQuery, 9);
    CHECK_OFFSET(bufferDeviceAddress, 10);
    CHECK_OFFSET(maxRecursionDepth, 11);
    CHECK_OFFSET(shaderGroupHandleSize, 12);
    CHECK_OFFSET(shaderGroupBaseAlignment, 13);

#undef CHECK_OFFSET

    std::cout << "[OK] ABI layout is 56 bytes and field offsets match\n";

    static_assert(sizeof(CloiSimRtMeshDesc) == 40);
    static_assert(offsetof(CloiSimRtMeshDesc, meshId) == 0);
    static_assert(offsetof(CloiSimRtMeshDesc, vertices) == 8);
    static_assert(offsetof(CloiSimRtMeshDesc, vertexCount) == 16);
    static_assert(offsetof(CloiSimRtMeshDesc, indices) == 24);
    static_assert(offsetof(CloiSimRtMeshDesc, indexCount) == 32);

    static_assert(sizeof(CloiSimRtInstanceDesc) == 64);
    static_assert(offsetof(CloiSimRtInstanceDesc, meshId) == 0);
    static_assert(offsetof(CloiSimRtInstanceDesc, transform) == 8);
    static_assert(offsetof(CloiSimRtInstanceDesc, instanceId) == 56);
    static_assert(offsetof(CloiSimRtInstanceDesc, mask) == 60);

    static_assert(offsetof(CloiSimRtLidarParams, samplesH) == 0);
    static_assert(offsetof(CloiSimRtLidarParams, angleMinH) == 8);
    static_assert(offsetof(CloiSimRtLidarParams, rangeMin) == 24);
    static_assert(offsetof(CloiSimRtLidarParams, sensorPosition) == 36);
    static_assert(offsetof(CloiSimRtLidarParams, sensorRight) == 48);
    static_assert(offsetof(CloiSimRtLidarParams, sensorUp) == 60);
    static_assert(offsetof(CloiSimRtLidarParams, sensorForward) == 72);
    static_assert(offsetof(CloiSimRtLidarParams, selfExclusionId) == 84);
    static_assert(offsetof(CloiSimRtLidarParams, maxSelfHitRetraces) == 88);

    static_assert(offsetof(CloiSimRtLidarTraceRequest, nativeOutputBuffer) == 0);
    static_assert(offsetof(CloiSimRtLidarTraceRequest, outputElementCount) == 8);
    static_assert(offsetof(CloiSimRtLidarTraceRequest, params) == 16);

    std::cout << "[OK] mesh/instance/lidar ABI struct layouts match\n";
    return 0;
}
