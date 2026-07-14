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
    return 0;
}
