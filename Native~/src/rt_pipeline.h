#pragma once
#include "vulkan_dispatch.h"
#include <vulkan/vulkan.h>
#include <cstdint>
#include <vector>
class RtPipeline {
public:
 ~RtPipeline();
 bool Create(VkPhysicalDevice physicalDevice,VkDevice device,VulkanDispatch* dispatch,const std::vector<uint32_t>& raygen,const std::vector<uint32_t>& miss,const std::vector<uint32_t>& closestHit);
 void Destroy();
 VkPipeline Handle() const{return pipeline_;} VkPipelineLayout Layout() const{return layout_;} VkDescriptorSetLayout DescriptorSetLayout() const{return descriptorSetLayout_;}
 uint32_t GroupCount() const{return 3;} const VkPhysicalDeviceRayTracingPipelinePropertiesKHR& Properties() const{return properties_;}
private:
 VkShaderModule CreateShader(const std::vector<uint32_t>& code) const;
 VkDevice device_=VK_NULL_HANDLE; VulkanDispatch* dispatch_=nullptr; VkDescriptorSetLayout descriptorSetLayout_=VK_NULL_HANDLE; VkPipelineLayout layout_=VK_NULL_HANDLE; VkPipeline pipeline_=VK_NULL_HANDLE; VkPhysicalDeviceRayTracingPipelinePropertiesKHR properties_{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
};
