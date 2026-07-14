#include "depth_trace_recorder.h"

#include <array>

bool DepthTraceRecorder::Initialize(
    VkDevice device,
    const RtPipeline* pipeline,
    const ShaderBindingTable* sbt)
{
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

bool DepthTraceRecorder::EnsureDescriptorResources()
{
    std::array<VkDescriptorPoolSize, 2> sizes{};
    sizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    sizes[0].descriptorCount = 1;
    sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    sizes[1].descriptorCount = 1;

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

bool DepthTraceRecorder::UpdateDescriptors(
    VkAccelerationStructureKHR tlas,
    VkImageView outputImageView)
{
    if (tlas == VK_NULL_HANDLE ||
        outputImageView == VK_NULL_HANDLE ||
        descriptorSet_ == VK_NULL_HANDLE)
    {
        return false;
    }

    VkWriteDescriptorSetAccelerationStructureKHR asInfo{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    asInfo.accelerationStructureCount = 1;
    asInfo.pAccelerationStructures = &tlas;

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageView = outputImageView;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    std::array<VkWriteDescriptorSet, 2> writes{};
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
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    writes[1].pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(
        device_,
        static_cast<uint32_t>(writes.size()),
        writes.data(),
        0,
        nullptr);

    return true;
}

bool DepthTraceRecorder::Record(
    VkCommandBuffer commandBuffer,
    VkAccelerationStructureKHR tlas,
    VkImageView outputImageView,
    uint32_t width,
    uint32_t height)
{
    if (commandBuffer == VK_NULL_HANDLE ||
        width == 0 ||
        height == 0 ||
        pipeline_ == nullptr ||
        sbt_ == nullptr ||
        !UpdateDescriptors(tlas, outputImageView))
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
        width,
        height,
        1);

    return true;
}

void DepthTraceRecorder::Shutdown()
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
