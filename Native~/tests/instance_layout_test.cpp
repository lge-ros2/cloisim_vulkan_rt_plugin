#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

int main()
{
    static_assert(std::is_standard_layout_v<VkTransformMatrixKHR>);
    static_assert(std::is_standard_layout_v<
        VkAccelerationStructureInstanceKHR>);
    static_assert(sizeof(VkTransformMatrixKHR) == 12 * sizeof(float));
    static_assert(alignof(VkAccelerationStructureInstanceKHR) >= 8);

    VkAccelerationStructureInstanceKHR instance{};
    instance.transform.matrix[0][0] = 1.0F;
    instance.transform.matrix[1][1] = 1.0F;
    instance.transform.matrix[2][2] = 1.0F;
    instance.instanceCustomIndex = 7;
    instance.mask = 0xff;
    instance.instanceShaderBindingTableRecordOffset = 0;
    instance.flags =
        VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instance.accelerationStructureReference = 0x12345678ULL;

    if (instance.mask != 0xff ||
        instance.instanceCustomIndex != 7 ||
        instance.accelerationStructureReference != 0x12345678ULL)
    {
        std::cerr << "[FAIL] TLAS instance field round-trip failed\n";
        return 1;
    }

    std::cout << "[OK] TLAS instance layout validation passed\n";
    return 0;
}
