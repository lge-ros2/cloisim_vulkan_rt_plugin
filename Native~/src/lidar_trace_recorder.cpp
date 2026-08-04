#include "lidar_trace_recorder.h"

#include "vulkan_dispatch.h"

#include <array>

bool LidarTraceRecorder::Initialize(
    VkDevice device,
    const RtPipeline* pipeline,
    const ShaderBindingTable* sbt)
{
    // Same reuse rationale as DepthTraceRecorder::Initialize: pipeline/sbt
    // are persistent objects owned by LidarPipelineResources, so an
    // identical combination reuses the existing descriptor pool/set rather
    // than destroying and recreating it every frame (which would risk
    // destroying a pool still referenced by an in-flight trace).
    if (device != VK_NULL_HANDLE &&
        device == device_ &&
        pipeline == pipeline_ &&
        sbt == sbt_ &&
        descriptorSet_ != VK_NULL_HANDLE)
    {
        return true;
    }

    Shutdown();

    if (device == VK_NULL_HANDLE ||
        pipeline == nullptr ||
        pipeline->Handle() == VK_NULL_HANDLE ||
        pipeline->Layout() == VK_NULL_HANDLE ||
        pipeline->DescriptorSetLayout() == VK_NULL_HANDLE ||
        sbt == nullptr)
    {
        return false;
    }

    device_ = device;
    pipeline_ = pipeline;
    sbt_ = sbt;
    return EnsureDescriptorResources();
}

bool LidarTraceRecorder::EnsureDescriptorResources()
{
    std::array<VkDescriptorPoolSize, 3> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    sizes[0].descriptorCount = 1;
    sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sizes[1].descriptorCount = 1;
    sizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    sizes[2].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(sizes.size());
    poolInfo.pPoolSizes = sizes.data();

    if (vkCreateDescriptorPool(
            device_,
            &poolInfo,
            nullptr,
            &descriptorPool_) != VK_SUCCESS)
    {
        return false;
    }

    const VkDescriptorSetLayout layout =
        pipeline_->DescriptorSetLayout();

    VkDescriptorSetAllocateInfo allocateInfo{
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(
            device_,
            &allocateInfo,
            &descriptorSet_) != VK_SUCCESS)
    {
        Shutdown();
        return false;
    }

    return true;
}

bool LidarTraceRecorder::UpdateDescriptors(
    VkAccelerationStructureKHR tlas,
    VkBuffer outputBuffer,
    VkDeviceSize outputBufferSize,
    VkBuffer paramsUbo,
    VkDeviceSize paramsUboSize)
{
    if (tlas == VK_NULL_HANDLE ||
        outputBuffer == VK_NULL_HANDLE ||
        paramsUbo == VK_NULL_HANDLE ||
        descriptorSet_ == VK_NULL_HANDLE)
    {
        return false;
    }

    VkWriteDescriptorSetAccelerationStructureKHR asInfo{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asInfo.accelerationStructureCount = 1;
    asInfo.pAccelerationStructures = &tlas;

    VkDescriptorBufferInfo outputInfo{};
    outputInfo.buffer = outputBuffer;
    outputInfo.offset = 0;
    outputInfo.range = outputBufferSize;

    VkDescriptorBufferInfo uboInfo{};
    uboInfo.buffer = paramsUbo;
    uboInfo.offset = 0;
    uboInfo.range = paramsUboSize;

    std::array<VkWriteDescriptorSet, 3> writes{};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = &asInfo;
    writes[0].dstSet = descriptorSet_;
    writes[0].dstBinding = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType =
        VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = descriptorSet_;
    writes[1].dstBinding = 1;
    writes[1].descriptorCount = 1;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    writes[1].pBufferInfo = &outputInfo;

    writes[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[2].dstSet = descriptorSet_;
    writes[2].dstBinding = 2;
    writes[2].descriptorCount = 1;
    writes[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[2].pBufferInfo = &uboInfo;

    vkUpdateDescriptorSets(
        device_,
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr);

    return true;
}

bool LidarTraceRecorder::Record(
    VkCommandBuffer commandBuffer,
    VkAccelerationStructureKHR tlas,
    VkBuffer outputBuffer,
    VkDeviceSize outputBufferSize,
    VkBuffer paramsUbo,
    VkDeviceSize paramsUboSize,
    uint32_t samplesH,
    uint32_t samplesV)
{
    if (commandBuffer == VK_NULL_HANDLE ||
        samplesH == 0 ||
        samplesV == 0 ||
        pipeline_ == nullptr ||
        sbt_ == nullptr ||
        !UpdateDescriptors(tlas, outputBuffer, outputBufferSize, paramsUbo, paramsUboSize))
    {
        return false;
    }

    vkCmdBindPipeline(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipeline_->Handle());

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
        pipeline_->Layout(),
        0,
        1,
        &descriptorSet_,
        0,
        nullptr);

    const VkStridedDeviceAddressRegionKHR callable{};
    VulkanDispatch::Instance().cmdTraceRays(
        commandBuffer,
        &sbt_->Raygen(),
        &sbt_->Miss(),
        &sbt_->Hit(),
        &callable,
        samplesH,
        samplesV,
        1);

    return true;
}

void LidarTraceRecorder::Shutdown()
{
    descriptorSet_ = VK_NULL_HANDLE;

    if (device_ != VK_NULL_HANDLE &&
        descriptorPool_ != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
    }

    descriptorPool_ = VK_NULL_HANDLE;
    pipeline_ = nullptr;
    sbt_ = nullptr;
    device_ = VK_NULL_HANDLE;
}
