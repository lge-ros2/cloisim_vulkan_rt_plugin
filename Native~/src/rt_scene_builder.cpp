#include "rt_scene_builder.h"

bool RtSceneBuilder::Initialize(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch)
{
    physicalDevice_ = physicalDevice;
    device_ = device;
    dispatch_ = dispatch;

    return physicalDevice_ != VK_NULL_HANDLE &&
        device_ != VK_NULL_HANDLE &&
        dispatch_ != nullptr &&
        dispatch_->IsLoaded();
}

bool RtSceneBuilder::CreateBlas(
    VkCommandBuffer commandBuffer,
    const BlasTriangleInput& input,
    AccelerationStructure& blas,
    GpuBuffer& scratch)
{
    if (input.vertexAddress == 0 ||
        input.indexAddress == 0 ||
        input.vertexCount == 0 ||
        input.vertexStride == 0 ||
        input.indexCount < 3)
    {
        return false;
    }

    VkAccelerationStructureGeometryTrianglesDataKHR triangles{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR};
    triangles.vertexFormat = input.vertexFormat;
    triangles.vertexData.deviceAddress = input.vertexAddress;
    triangles.vertexStride = input.vertexStride;
    triangles.maxVertex = input.vertexCount - 1;
    triangles.indexType = input.indexType;
    triangles.indexData.deviceAddress = input.indexAddress;

    VkAccelerationStructureGeometryKHR geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
    geometry.flags = input.geometryFlags;
    geometry.geometry.triangles = triangles;

    return CreateAndRecord(
        commandBuffer,
        VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
        geometry,
        input.indexCount / 3,
        blas,
        scratch);
}

bool RtSceneBuilder::CreateTlas(
    VkCommandBuffer commandBuffer,
    VkDeviceAddress instanceAddress,
    uint32_t instanceCount,
    AccelerationStructure& tlas,
    GpuBuffer& scratch)
{
    if (instanceAddress == 0 || instanceCount == 0)
        return false;

    VkAccelerationStructureGeometryInstancesDataKHR instances{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR};
    instances.arrayOfPointers = VK_FALSE;
    instances.data.deviceAddress = instanceAddress;

    VkAccelerationStructureGeometryKHR geometry{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR};
    geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
    geometry.geometry.instances = instances;

    return CreateAndRecord(
        commandBuffer,
        VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
        geometry,
        instanceCount,
        tlas,
        scratch);
}

bool RtSceneBuilder::CreateAndRecord(
    VkCommandBuffer commandBuffer,
    VkAccelerationStructureTypeKHR type,
    const VkAccelerationStructureGeometryKHR& geometry,
    uint32_t primitiveCount,
    AccelerationStructure& output,
    GpuBuffer& scratch)
{
    if (commandBuffer == VK_NULL_HANDLE ||
        primitiveCount == 0 ||
        physicalDevice_ == VK_NULL_HANDLE ||
        device_ == VK_NULL_HANDLE ||
        dispatch_ == nullptr ||
        !dispatch_->IsLoaded())
    {
        return false;
    }

    VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR};
    buildInfo.type = type;
    buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
    buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
    buildInfo.geometryCount = 1;
    buildInfo.pGeometries = &geometry;

    VkAccelerationStructureBuildSizesInfoKHR sizes{
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
    dispatch_->getAccelerationStructureBuildSizes(
        device_,
        VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
        &buildInfo,
        &primitiveCount,
        &sizes);

    if (sizes.accelerationStructureSize == 0 ||
        sizes.buildScratchSize == 0)
    {
        return false;
    }

    if (!output.Create(
            physicalDevice_,
            device_,
            dispatch_,
            type,
            sizes.accelerationStructureSize))
    {
        return false;
    }

    const VkBufferUsageFlags scratchUsage =
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    if (!scratch.Create(
            physicalDevice_,
            device_,
            dispatch_,
            sizes.buildScratchSize,
            scratchUsage,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
    {
        output.Destroy();
        return false;
    }

    buildInfo.dstAccelerationStructure = output.Handle();
    buildInfo.scratchData.deviceAddress = scratch.DeviceAddress();

    VkAccelerationStructureBuildRangeInfoKHR range{};
    range.primitiveCount = primitiveCount;
    range.primitiveOffset = 0;
    range.firstVertex = 0;
    range.transformOffset = 0;

    const VkAccelerationStructureBuildRangeInfoKHR* ranges[] = {
        &range};

    dispatch_->cmdBuildAccelerationStructures(
        commandBuffer,
        1,
        &buildInfo,
        ranges);

    return true;
}

void RtSceneBuilder::InsertBuildBarrier(
    VkCommandBuffer commandBuffer)
{
    if (commandBuffer == VK_NULL_HANDLE)
        return;

    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    barrier.dstAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        0,
        1,
        &barrier,
        0,
        nullptr,
        0,
        nullptr);
}
