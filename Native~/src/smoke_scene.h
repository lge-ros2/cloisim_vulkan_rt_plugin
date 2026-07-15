#pragma once
#include "acceleration_structure.h"
#include "deferred_release_queue.h"
#include "gpu_buffer.h"
#include "rt_scene_builder.h"
#include "tlas_instance_buffer.h"
#include <cstdint>
#include <vulkan/vulkan.h>
class SmokeScene {
public:
 bool Initialize(VkPhysicalDevice physicalDevice,VkDevice device,VulkanDispatch* dispatch);
 bool RecordBuild(VkCommandBuffer commandBuffer);
 void Destroy();
 // 현재 소유 중인 BLAS/TLAS/버퍼를 즉시 파괴하지 않고 DeferredReleaseQueue로
 // 이전한다. 이전 프레임에서 빌드/트레이스에 사용된 GPU 리소스가 아직
 // in-flight일 수 있으므로, safeFrameNumber에 도달하기 전까지 파괴를
 // 미뤄야 한다. 호출 후 인스턴스는 미초기화 상태가 된다.
 void RetireInto(DeferredReleaseQueue& queue, uint64_t lastUsedFrame);
 bool IsReady() const{return ready_;}
 VkAccelerationStructureKHR Tlas() const{return tlas_.Handle();}
 GpuBuffer ReleaseBlasScratch(); GpuBuffer ReleaseTlasScratch();
private:
 VkPhysicalDevice physicalDevice_=VK_NULL_HANDLE; VkDevice device_=VK_NULL_HANDLE; VulkanDispatch* dispatch_=nullptr; RtSceneBuilder builder_;
 GpuBuffer vertices_,indices_,blasScratch_,tlasScratch_; TlasInstanceBuffer instances_; AccelerationStructure blas_,tlas_; bool ready_=false;
};
