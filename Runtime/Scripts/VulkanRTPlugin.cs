using System;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace CLOiSim.VulkanRT
{
    public static class VulkanRTPlugin
    {
        private const string LibraryName = "cloisim_vulkan_rt";

        public const uint ExpectedAbiVersion = 2;

        [StructLayout(LayoutKind.Sequential)]
        public struct Capabilities
        {
            public uint abiVersion;
            public uint apiVersion;
            public uint vendorId;
            public uint deviceId;
            public uint pluginLoaded;
            public uint vulkanDeviceReady;
            public uint dispatchLoaded;
            public uint accelerationStructure;
            public uint rayTracingPipeline;
            public uint rayQuery;
            public uint bufferDeviceAddress;
            public uint maxRecursionDepth;
            public uint shaderGroupHandleSize;
            public uint shaderGroupBaseAlignment;
        }

        [DllImport(LibraryName)]
        private static extern uint CLOISimRt_GetAbiVersion();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_GetCapabilities(out Capabilities capabilities);

        [DllImport(LibraryName)]
        private static extern IntPtr CLOISimRt_GetRenderEventFunc();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_IsNativeBackendAvailable();

        [DllImport(
            LibraryName,
            CharSet = CharSet.Ansi)]
        private static extern int CLOISimRt_SetShaderDirectory(
            string path);

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_InitializeDepthPipeline();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_IsDepthPipelineReady();

        public static bool InitializeDepthPipeline(
            string shaderDirectory)
        {
            if (!IsVulkan ||
                string.IsNullOrEmpty(shaderDirectory))
            {
                return false;
            }

            try
            {
                return
                    CLOISimRt_SetShaderDirectory(
                        shaderDirectory) == 0 &&
                    CLOISimRt_InitializeDepthPipeline() == 0 &&
                    CLOISimRt_IsDepthPipelineReady() == 1;
            }
            catch (DllNotFoundException)
            {
                return false;
            }
            catch (EntryPointNotFoundException)
            {
                return false;
            }
        }

        public static bool IsDepthPipelineReady
        {
            get
            {
                try
                {
                    return CLOISimRt_IsDepthPipelineReady() == 1;
                }
                catch (DllNotFoundException)
                {
                    return false;
                }
                catch (EntryPointNotFoundException)
                {
                    return false;
                }
            }
        }

        public static bool IsVulkan =>
            SystemInfo.graphicsDeviceType == GraphicsDeviceType.Vulkan;

        public static bool TryGetCapabilities(out Capabilities capabilities)
        {
            capabilities = default;

            if (!IsVulkan)
                return false;

            try
            {
                return CLOISimRt_GetAbiVersion() == ExpectedAbiVersion &&
                       CLOISimRt_GetCapabilities(out capabilities) == 0;
            }
            catch (DllNotFoundException)
            {
                return false;
            }
            catch (EntryPointNotFoundException)
            {
                return false;
            }
        }

        public static bool IsNativeBackendAvailable
        {
            get
            {
                if (!IsVulkan) return false;
                try { return CLOISimRt_GetAbiVersion() == ExpectedAbiVersion && CLOISimRt_IsNativeBackendAvailable() == 1; }
                catch (DllNotFoundException) { return false; }
                catch (EntryPointNotFoundException) { return false; }
            }
        }

        public static IntPtr GetRenderEventFunc()
        {
            return CLOISimRt_GetRenderEventFunc();
        }
    }
}
