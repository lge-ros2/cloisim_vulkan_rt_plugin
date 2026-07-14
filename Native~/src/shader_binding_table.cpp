#include "shader_binding_table.h"
#include <algorithm>
#include <cstring>
#include <vector>
static VkDeviceSize Align(VkDeviceSize v,VkDeviceSize a){return (v+a-1)&~(a-1);}
bool ShaderBindingTable::Create(VkPhysicalDevice pd,VkDevice d,VulkanDispatch* dispatch,const RtPipeline& p){Destroy();auto prop=p.Properties();VkDeviceSize stride=Align(prop.shaderGroupHandleSize,prop.shaderGroupHandleAlignment);VkDeviceSize region=Align(stride,prop.shaderGroupBaseAlignment);std::vector<unsigned char> handles(prop.shaderGroupHandleSize*p.GroupCount());if(dispatch->getRayTracingShaderGroupHandles(d,p.Handle(),0,p.GroupCount(),handles.size(),handles.data())!=VK_SUCCESS)return false;VkDeviceSize total=region*3;if(!buffer_.Create(pd,d,dispatch,total,VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))return false;std::vector<unsigned char> data(total);for(uint32_t i=0;i<3;i++)std::memcpy(data.data()+region*i,handles.data()+prop.shaderGroupHandleSize*i,prop.shaderGroupHandleSize);if(!buffer_.Upload(data.data(),total)){Destroy();return false;}auto base=buffer_.DeviceAddress();raygen_={base,stride,stride};miss_={base+region,stride,stride};hit_={base+region*2,stride,stride};return true;}
void ShaderBindingTable::Destroy(){buffer_.Destroy();raygen_={};miss_={};hit_={};}
