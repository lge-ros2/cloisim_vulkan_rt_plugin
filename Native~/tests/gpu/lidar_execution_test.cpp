#include "acceleration_structure.h"
#include "gpu_buffer.h"
#include "lidar_params_gpu.h"
#include "lidar_trace_recorder.h"
#include "rt_descriptor_bindings.h"
#include "rt_pipeline.h"
#include "rt_scene_builder.h"
#include "shader_binding_table.h"
#include "shader_loader.h"
#include "tlas_instance_buffer.h"
#include "vulkan_dispatch.h"
#include "vulkan_memory.h"

#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

namespace
{
constexpr int kSkip = 77;
constexpr uint32_t kSamplesH = 2;
constexpr uint32_t kSamplesV = 1;

bool HasExtension(
    const std::vector<VkExtensionProperties>& properties,
    const char* name)
{
    return std::any_of(
        properties.begin(), properties.end(),
        [name](const auto& property)
        {
            return std::strcmp(property.extensionName, name) == 0;
        });
}

int Skip(const char* reason)
{
    std::cout << "[SKIP] " << reason << '\n';
    return kSkip;
}

int Fail(const char* reason)
{
    std::cerr << "[FAIL] " << reason << '\n';
    return 1;
}
}

int main()
{
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "cloisim_rt_lidar_execution_test";
    app.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceInfo{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &app;

    VkInstance instance = VK_NULL_HANDLE;
    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
        return Skip("Vulkan 1.2 instance unavailable");

    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::vector<VkPhysicalDevice> candidates(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, candidates.data());

    const std::array<const char*, 4> extensions = {
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME};

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t queueFamily = 0;

    for (auto candidate : candidates)
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(
            candidate, nullptr, &extensionCount, nullptr);
        std::vector<VkExtensionProperties> available(extensionCount);
        vkEnumerateDeviceExtensionProperties(
            candidate, nullptr, &extensionCount, available.data());
        if (!std::all_of(
                extensions.begin(), extensions.end(),
                [&available](const char* name)
                {
                    return HasExtension(available, name);
                }))
            continue;

        VkPhysicalDeviceBufferDeviceAddressFeatures bda{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
        VkPhysicalDeviceAccelerationStructureFeaturesKHR as{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
        VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
        VkPhysicalDeviceFeatures2 features{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
        features.pNext = &bda;
        bda.pNext = &as;
        as.pNext = &rt;
        vkGetPhysicalDeviceFeatures2(candidate, &features);
        if (!bda.bufferDeviceAddress ||
            !as.accelerationStructure ||
            !rt.rayTracingPipeline)
            continue;

        uint32_t queueCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate, &queueCount, nullptr);
        std::vector<VkQueueFamilyProperties> queues(queueCount);
        vkGetPhysicalDeviceQueueFamilyProperties(
            candidate, &queueCount, queues.data());
        for (uint32_t index = 0; index < queueCount; ++index)
        {
            if ((queues[index].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0)
            {
                physicalDevice = candidate;
                queueFamily = index;
                break;
            }
        }
        if (physicalDevice != VK_NULL_HANDLE)
            break;
    }

    if (physicalDevice == VK_NULL_HANDLE)
    {
        vkDestroyInstance(instance, nullptr);
        return Skip("no KHR ray tracing device");
    }

    float priority = 1.0F;
    VkDeviceQueueCreateInfo queueInfo{
        VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = queueFamily;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priority;

    VkPhysicalDeviceBufferDeviceAddressFeatures bda{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
    bda.bufferDeviceAddress = VK_TRUE;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR as{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    as.accelerationStructure = VK_TRUE;
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    rt.rayTracingPipeline = VK_TRUE;
    bda.pNext = &as;
    as.pNext = &rt;

    VkDeviceCreateInfo deviceInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceInfo.pNext = &bda;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    deviceInfo.enabledExtensionCount = extensions.size();
    deviceInfo.ppEnabledExtensionNames = extensions.data();

    VkDevice device = VK_NULL_HANDLE;
    if (vkCreateDevice(
            physicalDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS)
    {
        vkDestroyInstance(instance, nullptr);
        return Fail("vkCreateDevice failed");
    }

    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamily, 0, &queue);

    auto& dispatch = VulkanDispatch::Instance();
    if (!dispatch.Load(instance, device, vkGetInstanceProcAddr))
        return Fail("dispatch load failed");

    VkCommandPoolCreateInfo poolInfo{
        VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    poolInfo.queueFamilyIndex = queueFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VkCommandPool commandPool = VK_NULL_HANDLE;
    if (vkCreateCommandPool(
            device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        return Fail("command pool creation failed");

    VkCommandBufferAllocateInfo commandInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    commandInfo.commandPool = commandPool;
    commandInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandInfo.commandBufferCount = 1;
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    if (vkAllocateCommandBuffers(
            device, &commandInfo, &commandBuffer) != VK_SUCCESS)
        return Fail("command buffer allocation failed");

    // A large ground-plane-like quad (two triangles) at a known distance
    // (z=2) along the sensor's forward axis, big enough that a ray fired
    // straight along +Z definitely hits it.
    const std::array<float, 12> vertices = {
        -50.0F, -50.0F, 2.0F,
         50.0F, -50.0F, 2.0F,
         50.0F,  50.0F, 2.0F,
        -50.0F,  50.0F, 2.0F};
    const std::array<uint32_t, 6> indices = {0, 1, 2, 0, 2, 3};
    const auto geometryUsage =
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    if (!vertexBuffer.Create(
            physicalDevice, device, &dispatch, sizeof(vertices),
            geometryUsage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !vertexBuffer.Upload(vertices.data(), sizeof(vertices)) ||
        !indexBuffer.Create(
            physicalDevice, device, &dispatch, sizeof(indices),
            geometryUsage,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !indexBuffer.Upload(indices.data(), sizeof(indices)))
        return Fail("geometry buffer creation failed");

    RtSceneBuilder builder;
    if (!builder.Initialize(physicalDevice, device, &dispatch))
        return Fail("scene builder initialization failed");

    VkCommandBufferBeginInfo beginInfo{
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    BlasTriangleInput input{};
    input.vertexAddress = vertexBuffer.DeviceAddress();
    input.indexAddress = indexBuffer.DeviceAddress();
    input.vertexCount = 4;
    input.vertexStride = 3 * sizeof(float);
    input.indexCount = 6;

    AccelerationStructure blas;
    GpuBuffer blasScratch;
    if (!builder.CreateBlas(commandBuffer, input, blas, blasScratch))
        return Fail("BLAS record failed");

    VkMemoryBarrier buildBarrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    buildBarrier.srcAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
    buildBarrier.dstAccessMask =
        VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
        0, 1, &buildBarrier, 0, nullptr, 0, nullptr);

    // instanceCustomIndex intentionally != the test's selfExclusionId (0)
    // below, so the lidar shader's self-hit retrace logic never engages.
    VkAccelerationStructureInstanceKHR instanceData{};
    instanceData.transform.matrix[0][0] = 1.0F;
    instanceData.transform.matrix[1][1] = 1.0F;
    instanceData.transform.matrix[2][2] = 1.0F;
    instanceData.instanceCustomIndex = 999;
    instanceData.mask = 0xff;
    instanceData.flags =
        VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    instanceData.accelerationStructureReference = blas.DeviceAddress();

    TlasInstanceBuffer instances;
    if (!instances.CreateAndUpload(
            physicalDevice, device, &dispatch, {instanceData}))
        return Fail("TLAS instance upload failed");

    AccelerationStructure tlas;
    GpuBuffer tlasScratch;
    if (!builder.CreateTlas(
            commandBuffer,
            instances.DeviceAddress(),
            instances.InstanceCount(),
            tlas,
            tlasScratch))
        return Fail("TLAS record failed");

    RtSceneBuilder::InsertBuildBarrier(commandBuffer);

    std::vector<uint32_t> raygen;
    std::vector<uint32_t> miss;
    std::vector<uint32_t> hit;
    const std::filesystem::path shaderDirectory =
        CLOISIM_RT_TEST_SHADER_DIR;
    if (!ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rgen.spv", raygen) ||
        !ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rmiss.spv", miss) ||
        !ShaderLoader::LoadSpirv(
            shaderDirectory / "lidar.rchit.spv", hit))
        return Fail("SPIR-V load failed");

    RtPipeline pipeline;
    if (!pipeline.Create(
            physicalDevice, device, &dispatch, raygen, miss, hit,
            LidarDescriptorBindings()))
        return Fail("RT pipeline creation failed");

    ShaderBindingTable sbt;
    if (!sbt.Create(physicalDevice, device, &dispatch, pipeline))
        return Fail("SBT creation failed");

    // Output storage buffer: kSamplesH * kSamplesV floats.
    const VkDeviceSize outputBytes = kSamplesH * kSamplesV * sizeof(float);
    GpuBuffer outputBuffer;
    if (!outputBuffer.Create(
            physicalDevice, device, &dispatch, outputBytes,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT))
        return Fail("output buffer creation failed");

    // Params UBO: ray0 points straight along +Z (forward) and hits the
    // ground plane at z=2; ray1 is rotated 180 degrees horizontally (away
    // from any geometry) and must miss.
    LidarParamsGpu params{};
    params.samplesH = kSamplesH;
    params.samplesV = kSamplesV;
    params.angleMinH = 0.0F;
    params.angleStepH = 3.14159265F;
    params.angleMinV = 0.0F;
    params.angleStepV = 0.0F;
    params.rangeMin = 0.01F;
    params.rangeMax = 1000.0F;
    params.rangeLinearResolution = 0.0F;
    params.sensorPositionX = 0.0F;
    params.sensorPositionY = 0.0F;
    params.sensorPositionZ = 0.0F;
    params.sensorRightX = 1.0F;
    params.sensorRightY = 0.0F;
    params.sensorRightZ = 0.0F;
    params.sensorUpX = 0.0F;
    params.sensorUpY = 1.0F;
    params.sensorUpZ = 0.0F;
    params.sensorForwardX = 0.0F;
    params.sensorForwardY = 0.0F;
    params.sensorForwardZ = 1.0F;
    params.selfExclusionId = 0;
    params.maxSelfHitRetraces = 8;

    GpuBuffer paramsUbo;
    if (!paramsUbo.Create(
            physicalDevice, device, &dispatch, sizeof(params),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) ||
        !paramsUbo.Upload(&params, sizeof(params)))
        return Fail("params UBO creation failed");

    LidarTraceRecorder recorder;
    if (!recorder.Initialize(device, &pipeline, &sbt) ||
        !recorder.Record(
            commandBuffer, tlas.Handle(),
            outputBuffer.Handle(), outputBuffer.Size(),
            paramsUbo.Handle(), paramsUbo.Size(),
            kSamplesH, kSamplesV))
        return Fail("trace record failed");

    VkBufferMemoryBarrier transferBarrier{VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER};
    transferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    transferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    transferBarrier.buffer = outputBuffer.Handle();
    transferBarrier.size = outputBytes;
    vkCmdPipelineBarrier(
        commandBuffer,
        VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0, 0, nullptr, 1, &transferBarrier, 0, nullptr);

    GpuBuffer readback;
    if (!readback.Create(
            physicalDevice, device, &dispatch, outputBytes,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
        return Fail("readback buffer creation failed");

    VkBufferCopy copy{};
    copy.size = outputBytes;
    vkCmdCopyBuffer(commandBuffer, outputBuffer.Handle(), readback.Handle(), 1, &copy);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
        return Fail("command buffer end failed");

    VkFenceCreateInfo fenceInfo{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(device, &fenceInfo, nullptr, &fence);
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &commandBuffer;
    if (vkQueueSubmit(queue, 1, &submit, fence) != VK_SUCCESS ||
        vkWaitForFences(device, 1, &fence, VK_TRUE, 10'000'000'000ULL) !=
            VK_SUCCESS)
        return Fail("queue submit or fence wait failed");

    void* mapped = nullptr;
    if (vkMapMemory(device, readback.Memory(), 0, outputBytes, 0, &mapped) !=
        VK_SUCCESS)
        return Fail("readback map failed");

    const auto* ranges = static_cast<const float*>(mapped);
    const float hitDistance = ranges[0];
    const float missDistance = ranges[1];
    vkUnmapMemory(device, readback.Memory());

    std::cout << "hit=" << hitDistance << " miss=" << missDistance << '\n';

    const bool hitValid = std::isfinite(hitDistance) &&
        hitDistance > 1.5F && hitDistance < 2.5F;
    const bool missIsNan = std::isnan(missDistance);

    const int result = hitValid && missIsNan
        ? 0
        : Fail("lidar output did not match hit/miss expectations");

    vkDeviceWaitIdle(device);
    vkDestroyFence(device, fence, nullptr);
    recorder.Shutdown();
    sbt.Destroy();
    pipeline.Destroy();
    tlas.Destroy();
    tlasScratch.Destroy();
    instances.Destroy();
    blas.Destroy();
    blasScratch.Destroy();
    indexBuffer.Destroy();
    vertexBuffer.Destroy();
    outputBuffer.Destroy();
    paramsUbo.Destroy();
    readback.Destroy();
    vkDestroyCommandPool(device, commandPool, nullptr);
    dispatch.Reset();
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);

    if (result == 0)
        std::cout << "[OK] standalone lidar BLAS/TLAS RT trace passed\n";
    return result;
}
