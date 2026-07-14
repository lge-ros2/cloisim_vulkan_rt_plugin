#pragma once
#include "gpu_buffer.h"
#include "rt_pipeline.h"
class ShaderBindingTable {public:bool Create(VkPhysicalDevice physicalDevice,VkDevice device,VulkanDispatch* dispatch,const RtPipeline& pipeline);void Destroy();const VkStridedDeviceAddressRegionKHR& Raygen()const{return raygen_;}const VkStridedDeviceAddressRegionKHR& Miss()const{return miss_;}const VkStridedDeviceAddressRegionKHR& Hit()const{return hit_;}private:GpuBuffer buffer_;VkStridedDeviceAddressRegionKHR raygen_{},miss_{},hit_{};};
