#pragma once
#include "acceleration_structure.h"
#include "gpu_buffer.h"
#include "rt_scene_builder.h"
#include "tlas_instance_buffer.h"
#include <vulkan/vulkan.h>
class SmokeScene {
public:
 bool Initialize(VkPhysicalDevice physicalDevice,VkDevice device,VulkanDispatch* dispatch);
 bool RecordBuild(VkCommandBuffer commandBuffer);
 void Destroy();
 bool IsReady() const{return ready_;}
 VkAccelerationStructureKHR Tlas() const{return tlas_.Handle();}
 GpuBuffer ReleaseBlasScratch(); GpuBuffer ReleaseTlasScratch();
private:
 VkPhysicalDevice physicalDevice_=VK_NULL_HANDLE; VkDevice device_=VK_NULL_HANDLE; VulkanDispatch* dispatch_=nullptr; RtSceneBuilder builder_;
 GpuBuffer vertices_,indices_,blasScratch_,tlasScratch_; TlasInstanceBuffer instances_; AccelerationStructure blas_,tlas_; bool ready_=false;
};
