#include "shader_binding_table.h"
#include <algorithm>
#include <cstring>
#include <vector>
static VkDeviceSize Align(VkDeviceSize v,VkDeviceSize a){return (v+a-1)&~(a-1);}
bool ShaderBindingTable::Create(VkPhysicalDevice pd,VkDevice d,VulkanDispatch* dispatch,const RtPipeline& p){Destroy();auto prop=p.Properties();VkDeviceSize stride=Align(prop.shaderGroupHandleSize,prop.shaderGroupHandleAlignment);VkDeviceSize region=Align(stride,prop.shaderGroupBaseAlignment);std::vector<unsigned char> handles(prop.shaderGroupHandleSize*p.GroupCount());if(dispatch->getRayTracingShaderGroupHandles(d,p.Handle(),0,p.GroupCount(),handles.size(),handles.data())!=VK_SUCCESS)return false;VkDeviceSize total=region*3;if(!buffer_.Create(pd,d,dispatch,total,VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR|VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))return false;std::vector<unsigned char> data(total);for(uint32_t i=0;i<3;i++)std::memcpy(data.data()+region*i,handles.data()+prop.shaderGroupHandleSize*i,prop.shaderGroupHandleSize);if(!buffer_.Upload(data.data(),total)){Destroy();return false;}auto base=buffer_.DeviceAddress();
 // Vulkan 스펙은 vkGetBufferDeviceAddressKHR가 반환하는 주소가
 // shaderGroupBaseAlignment에 정렬됨을 보장하지 않는다. 대부분의 드라이버에서
 // 우연히 정렬되지만, 정렬되지 않은 경우 GPU가 SBT를 잘못 해석해 조용한 데이터
 // 손상으로 이어질 수 있으므로 명시적으로 검증해 실패로 처리한다.
 if(prop.shaderGroupBaseAlignment==0||base%prop.shaderGroupBaseAlignment!=0){Destroy();return false;}
 raygen_={base,stride,stride};miss_={base+region,stride,stride};hit_={base+region*2,stride,stride};return true;}
void ShaderBindingTable::Destroy(){buffer_.Destroy();raygen_={};miss_={};hit_={};}
