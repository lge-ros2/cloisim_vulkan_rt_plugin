using System;
using System.Collections;
using System.IO;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;

namespace CLOiSim.VulkanRT
{
    public sealed class VulkanRTSmokeTest : MonoBehaviour
    {
        [StructLayout(LayoutKind.Sequential)]
        private struct DepthOutput
        {
            public IntPtr nativeTexture;
            public uint width;
            public uint height;
            public uint reserved;
        }

        private const string LibraryName = "cloisim_vulkan_rt";

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_SetDepthOutput(
            ref DepthOutput output);

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_IsSmokeSceneReady();

        [DllImport(LibraryName)]
        private static extern int CLOISimRt_GetLastTraceStatus();

        public IEnumerator Run(Action<bool, string> completed)
        {
            if (completed == null)
                yield break;

            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan)
            {
                completed(false, "Vulkan backend required");
                yield break;
            }

            var shaderDirectory =
                Environment.GetEnvironmentVariable(
                    "CLOISIM_RT_SHADER_DIR");

            if (string.IsNullOrWhiteSpace(shaderDirectory))
            {
                shaderDirectory = Path.GetFullPath(
                    Path.Combine(
                        Application.dataPath,
                        "..",
                        "Packages",
                        "com.lge-ros2.cloisim.vulkan-rt",
                        "Runtime",
                        "Shaders"));
            }

            if (!Directory.Exists(shaderDirectory))
            {
                completed(
                    false,
                    $"Shader directory not found: {shaderDirectory}");
                yield break;
            }

            if (!VulkanRTPlugin.InitializeDepthPipeline(
                    shaderDirectory))
            {
                completed(
                    false,
                    "Depth pipeline initialization failed");
                yield break;
            }

            var renderEventFunc =
                VulkanRTPlugin.RenderEventFunc;

            if (renderEventFunc == IntPtr.Zero)
            {
                completed(
                    false,
                    "Native render event function is unavailable");
                yield break;
            }

            var descriptor = new RenderTextureDescriptor(
                16,
                16,
                RenderTextureFormat.RFloat,
                0)
            {
                enableRandomWrite = true,
                msaaSamples = 1,
                volumeDepth = 1,
                dimension = TextureDimension.Tex2D,
            };

            var output = new RenderTexture(descriptor);

            try
            {
                if (!output.Create())
                {
                    completed(
                        false,
                        "RenderTexture creation failed");
                    yield break;
                }

                var request = new DepthOutput
                {
                    nativeTexture =
                        output.GetNativeTexturePtr(),
                    width = 16,
                    height = 16,
                    reserved = 0,
                };

                if (request.nativeTexture == IntPtr.Zero)
                {
                    completed(
                        false,
                        "Native texture pointer is null");
                    yield break;
                }

                if (CLOISimRt_SetDepthOutput(ref request) != 0)
                {
                    completed(
                        false,
                        "SetDepthOutput failed");
                    yield break;
                }

                GL.IssuePluginEvent(
                    renderEventFunc,
                    2);

                yield return new WaitForEndOfFrame();

                GL.IssuePluginEvent(
                    renderEventFunc,
                    3);

                yield return new WaitForEndOfFrame();

                if (CLOISimRt_IsSmokeSceneReady() != 1)
                {
                    completed(
                        false,
                        "Smoke scene is not ready");
                    yield break;
                }

                if (CLOISimRt_GetLastTraceStatus() != 1)
                {
                    completed(
                        false,
                        "Native trace was not recorded");
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
                    completed(
                        false,
                        "GPU readback failed");
                    yield break;
                }

                var data = readback.GetData<float>();

                if (data.Length < 16 * 16)
                {
                    completed(
                        false,
                        $"Unexpected readback length: {data.Length}");
                    yield break;
                }

                var center = data[8 * 16 + 8];
                var corner = data[0];

                var centerIsFinite =
                    !float.IsNaN(center) &&
                    !float.IsInfinity(center);

                var succeeded =
                    centerIsFinite &&
                    center > 1.5f &&
                    center < 3.0f &&
                    corner > 1.0e20f;

                completed(
                    succeeded,
                    $"center={center}, corner={corner}");
            }
            finally
            {
                output.Release();
                Destroy(output);
            }
        }
    }
}
