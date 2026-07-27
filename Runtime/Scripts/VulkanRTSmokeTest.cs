using System;
using System.Collections;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

#if UNITY_EDITOR
using UnityEditor.PackageManager;
#endif

namespace CLOiSim.VulkanRT
{
    public sealed class VulkanRTSmokeTest : MonoBehaviour
    {
        private const string LibraryName = "cloisim_vulkan_rt";
        private const string PackageId = "com.lge-ros2.cloisim.vulkan-rt";
        private const string ShaderDirectoryEnvironment =
            "CLOISIM_RT_SHADER_DIR";

        private const int Width = 16;
        private const int Height = 16;
        private const int BuildSmokeSceneEventId = 2;
        private const int TraceDepthEventId = 3;

        [StructLayout(LayoutKind.Sequential)]
        private struct DepthOutput
        {
            public IntPtr nativeTexture;
            public uint width;
            public uint height;
            public uint reserved;
        }

        [DllImport(
            LibraryName,
            CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_SetDepthOutput(
            ref DepthOutput output);

        [DllImport(
            LibraryName,
            CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_IsSmokeSceneReady();

        [DllImport(
            LibraryName,
            CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_GetLastTraceStatus();

        public IEnumerator Run(Action<bool, string> completed)
        {
            if (completed == null)
                yield break;

            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan)
            {
                completed(
                    false,
                    $"Vulkan backend required. Current backend: " +
                    $"{SystemInfo.graphicsDeviceType}");
                yield break;
            }

            if (!TryResolveShaderDirectory(out var shaderDirectory))
            {
                completed(
                    false,
                    "SPIR-V shader directory was not found. " +
                    "Set CLOISIM_RT_SHADER_DIR or ensure the package " +
                    "contains Runtime/Shaders.");
                yield break;
            }

            Debug.Log(
                $"[CLOiSim Vulkan RT] Shader directory: " +
                $"{shaderDirectory}",
                this);

            if (!VulkanRTPlugin.InitializeDepthPipeline(shaderDirectory))
            {
                completed(false, "Depth pipeline initialization failed");
                yield break;
            }

            var renderEventFunc = VulkanRTPlugin.RenderEventFunc;

            if (renderEventFunc == IntPtr.Zero)
            {
                completed(
                    false,
                    "Native render event function is unavailable");
                yield break;
            }

            var descriptor = new RenderTextureDescriptor(
                Width,
                Height,
                RenderTextureFormat.RFloat,
                0)
            {
                enableRandomWrite = true,
                msaaSamples = 1,
                volumeDepth = 1,
                dimension = TextureDimension.Tex2D,
                useMipMap = false,
                autoGenerateMips = false,
            };

            var output = new RenderTexture(descriptor)
            {
                name = "CLOiSim Vulkan RT Smoke Output",
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp,
            };

            try
            {
                if (!output.Create())
                {
                    completed(false, "RenderTexture creation failed");
                    yield break;
                }

                var nativeTexture = output.GetNativeTexturePtr();

                if (nativeTexture == IntPtr.Zero)
                {
                    completed(false, "RenderTexture native pointer is null");
                    yield break;
                }

                var request = new DepthOutput
                {
                    nativeTexture = nativeTexture,
                    width = Width,
                    height = Height,
                    reserved = 0,
                };

                if (CLOISimRt_SetDepthOutput(ref request) != 0)
                {
                    completed(false, "CLOISimRt_SetDepthOutput failed");
                    yield break;
                }

                GL.IssuePluginEvent(
                    renderEventFunc,
                    BuildSmokeSceneEventId);

                yield return new WaitForEndOfFrame();

                GL.IssuePluginEvent(
                    renderEventFunc,
                    TraceDepthEventId);

                yield return new WaitForEndOfFrame();

                if (CLOISimRt_IsSmokeSceneReady() != 1)
                {
                    completed(false, "Native smoke scene is not ready");
                    yield break;
                }

                if (CLOISimRt_GetLastTraceStatus() != 1)
                {
                    completed(false, "Native ray trace was not recorded");
                    yield break;
                }

                var readback = AsyncGPUReadback.Request(
                    output,
                    0,
                    TextureFormat.RFloat);

                while (!readback.done)
                    yield return null;

                if (readback.hasError)
                {
                    completed(false, "AsyncGPUReadback failed");
                    yield break;
                }

                var data = readback.GetData<float>();
                var expectedLength = Width * Height;

                if (data.Length < expectedLength)
                {
                    completed(
                        false,
                        $"Unexpected readback length: {data.Length}, " +
                        $"expected at least {expectedLength}");
                    yield break;
                }

                var centerIndex =
                    (Height / 2) * Width +
                    (Width / 2);

                var center = data[centerIndex];
                var corner = data[0];

                var centerIsFinite =
                    !float.IsNaN(center) &&
                    !float.IsInfinity(center);

                var centerIsValid =
                    centerIsFinite &&
                    center > 1.5f &&
                    center < 3.0f;

                var cornerIsMiss =
                    !float.IsNaN(corner) &&
                    corner > 1.0e20f;

                completed(
                    centerIsValid && cornerIsMiss,
                    $"center={center}, corner={corner}");
            }
            finally
            {
                output.Release();
                Destroy(output);
            }
        }

        private static bool TryResolveShaderDirectory(
            out string directory)
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
                File.Exists(Path.Combine(
                    directory,
                    "depth.rgen.spv")) &&
                File.Exists(Path.Combine(
                    directory,
                    "depth.rmiss.spv")) &&
                File.Exists(Path.Combine(
                    directory,
                    "depth.rchit.spv"));
        }

        private static void SetShaderDirectoryEnvironment(
            string directory)
        {
            directory = Path.GetFullPath(directory);

            Environment.SetEnvironmentVariable(
                ShaderDirectoryEnvironment,
                directory);
        }
    }
}
