using UnityEditor;
using UnityEngine;
using UnityEngine.Rendering;

namespace CLOiSim.VulkanRT.Editor
{
    internal static class VulkanRTPluginValidator
    {
        [MenuItem("CLOiSim/Vulkan RT/Validate Plugin")]
        private static void Validate()
        {
            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan)
            {
                Debug.LogError("[CLOiSim Vulkan RT] Vulkan graphics API is not active.");
                return;
            }

            if (!VulkanRTPlugin.TryGetCapabilities(out var capabilities))
            {
                Debug.LogError("[CLOiSim Vulkan RT] Native plugin or required ABI is unavailable.");
                return;
            }

            Debug.Log(
                $"[CLOiSim Vulkan RT] ABI={capabilities.abiVersion}, " +
                $"API=0x{capabilities.apiVersion:x}, " +
                $"Vendor=0x{capabilities.vendorId:x}, " +
                $"Device=0x{capabilities.deviceId:x}, " +
                $"AS={capabilities.accelerationStructure}, " +
                $"RT={capabilities.rayTracingPipeline}");
        }
    }
}
