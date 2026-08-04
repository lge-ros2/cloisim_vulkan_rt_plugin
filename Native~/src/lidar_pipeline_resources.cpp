#include "lidar_pipeline_resources.h"

#include "rt_descriptor_bindings.h"
#include "shader_loader.h"

#include <cstdint>
#include <vector>

bool LidarPipelineResources::Initialize(
    VkPhysicalDevice physicalDevice,
    VkDevice device,
    VulkanDispatch* dispatch,
    const std::filesystem::path& shaderDirectory)
{
    Shutdown();

    if (physicalDevice == VK_NULL_HANDLE ||
        device == VK_NULL_HANDLE ||
        dispatch == nullptr ||
        !dispatch->IsLoaded())
    {
        return false;
    }

    std::vector<uint32_t> raygen;
    std::vector<uint32_t> miss;
    std::vector<uint32_t> closestHit;

    if (!ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rgen.spv",
            raygen) ||
        !ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rmiss.spv",
            miss) ||
        !ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rchit.spv",
            closestHit))
    {
        return false;
    }

    if (!pipeline_.Create(
            physicalDevice,
            device,
            dispatch,
            raygen,
            miss,
            closestHit,
            LidarDescriptorBindings()))
    {
        return false;
    }

    if (!sbt_.Create(
            physicalDevice,
            device,
            dispatch,
            pipeline_))
    {
        pipeline_.Destroy();
        return false;
    }

    ready_ = true;
    return true;
}

void LidarPipelineResources::Shutdown()
{
    ready_ = false;
    sbt_.Destroy();
    pipeline_.Destroy();
}
