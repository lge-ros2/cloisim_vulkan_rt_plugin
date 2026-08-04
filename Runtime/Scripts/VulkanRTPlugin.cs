using System;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

#if UNITY_EDITOR
using UnityEditor.PackageManager;
#endif

namespace CLOiSim.VulkanRT
{
    public static class VulkanRTPlugin
    {
        private const string LibraryName = "cloisim_vulkan_rt";
        private const string PackageId = "com.lge-ros2.cloisim.vulkan-rt";
        private const string ShaderDirectoryEnvironment = "CLOISIM_RT_SHADER_DIR";

        [DllImport(LibraryName)]
        private static extern IntPtr CLOISimRt_GetRenderEventFunc();

        public const uint ExpectedAbiVersion = 3;

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
        private static extern int CLOISimRt_IsNativeBackendAvailable();

        // --- Real-scene geometry upload/instancing (see NativeScene, native side) ---

        [StructLayout(LayoutKind.Sequential)]
        private struct MeshDesc
        {
            public ulong meshId;
            public IntPtr vertices;
            public uint vertexCount;
            public IntPtr indices;
            public uint indexCount;
        }

        // Mirrors CloiSimRtInstanceDesc's memory layout (api.h) as individual
        // blittable fields, avoiding array-marshaling attributes so this
        // struct (and arrays of it) marshal via simple pinning.
        [StructLayout(LayoutKind.Sequential)]
        public struct NativeInstanceDesc
        {
            public ulong meshId;
            public float t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
            public uint instanceId;
            public uint mask;

            // Fills the transform fields from a row-major 3x4 (12-value)
            // array — see Matrix4x4.GetRow(0..2) on the caller side.
            public void SetTransform(
                float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23)
            {
                t0 = m00; t1 = m01; t2 = m02; t3 = m03;
                t4 = m10; t5 = m11; t6 = m12; t7 = m13;
                t8 = m20; t9 = m21; t10 = m22; t11 = m23;
            }
        }

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_UploadMesh(ref MeshDesc desc);

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_ReleaseMesh(ulong meshId);

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_SetSceneInstances(
            [In] NativeInstanceDesc[] instances,
            uint count);

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_IsSceneReady();

        public static bool UploadMesh(ulong meshId, float[] vertices, uint[] indices)
        {
            if (!IsVulkan || vertices == null || indices == null)
                return false;

            var verticesHandle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            var indicesHandle = GCHandle.Alloc(indices, GCHandleType.Pinned);
            try
            {
                var desc = new MeshDesc
                {
                    meshId = meshId,
                    vertices = verticesHandle.AddrOfPinnedObject(),
                    vertexCount = (uint)(vertices.Length / 3),
                    indices = indicesHandle.AddrOfPinnedObject(),
                    indexCount = (uint)indices.Length,
                };
                return CLOISimRt_UploadMesh(ref desc) == 0;
            }
            catch (DllNotFoundException)
            {
                return false;
            }
            catch (EntryPointNotFoundException)
            {
                return false;
            }
            finally
            {
                verticesHandle.Free();
                indicesHandle.Free();
            }
        }

        public static bool ReleaseMesh(ulong meshId)
        {
            try
            {
                return CLOISimRt_ReleaseMesh(meshId) == 0;
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

        public static bool SetSceneInstances(NativeInstanceDesc[] instances)
        {
            if (instances == null)
                return false;

            try
            {
                return CLOISimRt_SetSceneInstances(instances, (uint)instances.Length) == 0;
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

        public static bool IsSceneReady
        {
            get
            {
                try
                {
                    return CLOISimRt_IsSceneReady() == 1;
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

        // --- Lidar ray-trace pipeline ---

        [StructLayout(LayoutKind.Sequential)]
        public struct LidarParams
        {
            public uint samplesH, samplesV;
            public float angleMinH, angleStepH, angleMinV, angleStepV;
            public float rangeMin, rangeMax, rangeLinearResolution;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] sensorPosition;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] sensorRight;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] sensorUp;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public float[] sensorForward;

            public uint selfExclusionId;
            public uint maxSelfHitRetraces;
        }

        [StructLayout(LayoutKind.Sequential)]
        public struct LidarTraceRequest
        {
            public IntPtr nativeOutputBuffer;
            public uint outputElementCount;
            public uint reserved;
            public LidarParams parameters;
        }

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_InitializeLidarPipeline();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_IsLidarPipelineReady();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_GetLastLidarTraceStatus();

        public static bool InitializeLidarPipeline(string shaderDirectory)
        {
            if (!IsVulkan || string.IsNullOrEmpty(shaderDirectory))
                return false;

            try
            {
                return
                    CLOISimRt_SetShaderDirectory(shaderDirectory) == 0 &&
                    CLOISimRt_InitializeLidarPipeline() == 0 &&
                    CLOISimRt_IsLidarPipelineReady() == 1;
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

        public static bool IsLidarPipelineReady
        {
            get
            {
                try
                {
                    return CLOISimRt_IsLidarPipelineReady() == 1;
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

        public static int GetLastLidarTraceStatus()
        {
            try
            {
                return CLOISimRt_GetLastLidarTraceStatus();
            }
            catch (DllNotFoundException)
            {
                return 0;
            }
            catch (EntryPointNotFoundException)
            {
                return 0;
            }
        }

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


        public static IntPtr RenderEventFunc
        {
            get
            {
                try
                {
                    return CLOISimRt_GetRenderEventFunc();
                }
                catch (DllNotFoundException)
                {
                    return IntPtr.Zero;
                }
                catch (EntryPointNotFoundException)
                {
                    return IntPtr.Zero;
                }
            }
        }

        // --- SPIR-V shader directory resolution ---
        // Shared by every consumer that calls InitializeDepthPipeline/
        // InitializeLidarPipeline (both need the same Runtime/Shaders
        // directory): the smoke test, and cloisim's URTSensorManager.

        public static bool TryResolveShaderDirectory(out string directory)
        {
            directory = Environment.GetEnvironmentVariable(
                ShaderDirectoryEnvironment) ?? string.Empty;

            if (IsShaderDirectoryValid(directory))
            {
                SetShaderDirectoryEnvironment(directory);
                return true;
            }

#if UNITY_EDITOR
            var packageInfo = PackageInfo.FindForAssembly(
                typeof(VulkanRTPlugin).Assembly);

            if (packageInfo != null &&
                !string.IsNullOrWhiteSpace(packageInfo.resolvedPath))
            {
                directory = Path.Combine(
                    packageInfo.resolvedPath,
                    "Runtime",
                    "Shaders");

                if (IsShaderDirectoryValid(directory))
                {
                    SetShaderDirectoryEnvironment(directory);
                    return true;
                }
            }
#endif

            directory = Path.GetFullPath(
                Path.Combine(
                    Application.dataPath,
                    "..",
                    "Packages",
                    PackageId,
                    "Runtime",
                    "Shaders"));

            if (IsShaderDirectoryValid(directory))
            {
                SetShaderDirectoryEnvironment(directory);
                return true;
            }

            directory = string.Empty;
            return false;
        }

        private static bool IsShaderDirectoryValid(string directory)
        {
            if (string.IsNullOrWhiteSpace(directory) ||
                !Directory.Exists(directory))
            {
                return false;
            }

            return
                File.Exists(Path.Combine(directory, "depth.rgen.spv")) &&
                File.Exists(Path.Combine(directory, "depth.rmiss.spv")) &&
                File.Exists(Path.Combine(directory, "depth.rchit.spv"));
        }

        private static void SetShaderDirectoryEnvironment(string directory)
        {
            directory = Path.GetFullPath(directory);

            Environment.SetEnvironmentVariable(
                ShaderDirectoryEnvironment,
                directory);
        }
}
}
