#pragma once

#include "acceleration_structure.h"
#include "gpu_buffer.h"
#include "vulkan_dispatch.h"

#include <vulkan/vulkan.h>

#include <cstdint>

struct BlasTriangleInput
{
    VkDeviceAddress vertexAddress = 0;
    VkDeviceAddress indexAddress = 0;
    uint32_t vertexCount = 0;
    uint32_t vertexStride = 0;
    uint32_t indexCount = 0;
    VkFormat vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
    VkIndexType indexType = VK_INDEX_TYPE_UINT32;
    VkGeometryFlagsKHR geometryFlags =
        VK_GEOMETRY_OPAQUE_BIT_KHR;
};

class RtSceneBuilder
{
public:
    bool Initialize(
        VkPhysicalDevice physicalDevice,
        VkDevice device,
        VulkanDispatch* dispatch);

    bool CreateBlas(
        VkCommandBuffer commandBuffer,
        const BlasTriangleInput& input,
        AccelerationStructure& blas,
        GpuBuffer& scratch);

    bool CreateTlas(
        VkCommandBuffer commandBuffer,
        VkDeviceAddress instanceAddress,
        uint32_t instanceCount,
        AccelerationStructure& tlas,
        GpuBuffer& scratch);

    static void InsertBuildBarrier(VkCommandBuffer commandBuffer);

private:
    bool CreateAndRecord(
        VkCommandBuffer commandBuffer,
        VkAccelerationStructureTypeKHR type,
        const VkAccelerationStructureGeometryKHR& geometry,
        uint32_t primitiveCount,
        AccelerationStructure& output,
        GpuBuffer& scratch);

    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VulkanDispatch* dispatch_ = nullptr;
};
