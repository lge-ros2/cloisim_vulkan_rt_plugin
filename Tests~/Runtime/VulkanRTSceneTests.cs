using System;
using System.Collections;
using System.Runtime.InteropServices;
using CLOiSim.VulkanRT;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.Rendering;
using UnityEngine.TestTools;

namespace CLOiSim.VulkanRT.Tests
{
    // Exercises NativeScene's real multi-mesh BLAS-cache/TLAS path (added to
    // replace the former hard-coded one-triangle SmokeScene) end-to-end,
    // reusing the already-verified depth pipeline/trace as a known-good
    // raytracer — no lidar shader exists yet at this point in the plan, so
    // this is a pure scene-correctness check.
    public sealed class VulkanRTSceneTests
    {
        private const string LibraryName = "cloisim_vulkan_rt";
        private const int Width = 16;
        private const int Height = 16;
        private const int BuildSceneEventId = 2;
        private const int TraceDepthEventId = 3;

        private const ulong NearMeshId = 101;
        private const ulong FarMeshId = 102;

        [StructLayout(LayoutKind.Sequential)]
        private struct DepthOutput
        {
            public IntPtr nativeTexture;
            public uint width;
            public uint height;
            public uint reserved;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MeshDesc
        {
            public ulong meshId;
            public IntPtr vertices;
            public uint vertexCount;
            public IntPtr indices;
            public uint indexCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct InstanceDesc
        {
            public ulong meshId;
            public float t0, t1, t2, t3, t4, t5, t6, t7, t8, t9, t10, t11;
            public uint instanceId;
            public uint mask;
        }

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_UploadMesh(ref MeshDesc desc);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_SetSceneInstances(ref InstanceDesc instances, uint count);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_SetDepthOutput(ref DepthOutput output);

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_IsSceneReady();

        [DllImport(LibraryName, CallingConvention = CallingConvention.Cdecl)]
        private static extern int CLOISimRt_GetLastTraceStatus();

        private static bool UploadTriangleAtZ(ulong meshId, float z)
        {
            float[] vertices =
            {
                -0.5f, -0.5f, z,
                 0.5f, -0.5f, z,
                 0.0f,  0.5f, z,
            };
            int[] indices = { 0, 1, 2 };

            var verticesPtr = Marshal.AllocHGlobal(vertices.Length * sizeof(float));
            var indicesPtr = Marshal.AllocHGlobal(indices.Length * sizeof(int));
            try
            {
                Marshal.Copy(vertices, 0, verticesPtr, vertices.Length);
                Marshal.Copy(indices, 0, indicesPtr, indices.Length);

                var desc = new MeshDesc
                {
                    meshId = meshId,
                    vertices = verticesPtr,
                    vertexCount = 3,
                    indices = indicesPtr,
                    indexCount = 3,
                };
                return CLOISimRt_UploadMesh(ref desc) == 0;
            }
            finally
            {
                Marshal.FreeHGlobal(verticesPtr);
                Marshal.FreeHGlobal(indicesPtr);
            }
        }

        // Row-major 3x4 identity translated by (0,0,zOffset) — the mesh's own
        // local-space z already places it at a given depth, so a zOffset of 0
        // is a plain identity instance.
        private static bool SetSingleInstance(ulong meshId, float zOffset)
        {
            var instance = new InstanceDesc
            {
                meshId = meshId,
                t0 = 1f, t1 = 0f, t2 = 0f, t3 = 0f,
                t4 = 0f, t5 = 1f, t6 = 0f, t7 = zOffset,
                t8 = 0f, t9 = 0f, t10 = 1f, t11 = 0f,
                instanceId = 0,
                mask = 0xFF,
            };
            return CLOISimRt_SetSceneInstances(ref instance, 1) == 0;
        }

        private static IEnumerator BuildAndTrace(IntPtr renderEventFunc, DepthOutput output)
        {
            if (CLOISimRt_SetDepthOutput(ref output) != 0)
                Assert.Fail("CLOISimRt_SetDepthOutput failed");

            GL.IssuePluginEvent(renderEventFunc, BuildSceneEventId);
            yield return new WaitForEndOfFrame();

            GL.IssuePluginEvent(renderEventFunc, TraceDepthEventId);
            yield return new WaitForEndOfFrame();

            Assert.That(CLOISimRt_IsSceneReady(), Is.EqualTo(1), "native scene not ready");
            Assert.That(CLOISimRt_GetLastTraceStatus(), Is.EqualTo(1), "trace was not recorded");
        }

        // Polls .done in a loop (matching VulkanRTSmokeTest's own readback
        // pattern) rather than any blocking wait — AsyncGPUReadbackRequest
        // has no synchronous completion API.
        private static IEnumerator ReadCenterDepth(RenderTexture output, Action<float> onDone)
        {
            var readback = AsyncGPUReadback.Request(output, 0, TextureFormat.RFloat);
            while (!readback.done)
                yield return null;

            Assert.That(readback.hasError, Is.False, "AsyncGPUReadback failed");
            var data = readback.GetData<float>();
            onDone(data[(Height / 2) * Width + (Width / 2)]);
        }

        [UnityTest]
        public IEnumerator MultiMeshSceneAndInstanceMove_ProducesExpectedDepths()
        {
            if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan)
            {
                Assert.Ignore($"Vulkan backend required. Current backend: {SystemInfo.graphicsDeviceType}");
                yield break;
            }

            Assert.That(
                VulkanRTPlugin.TryResolveShaderDirectory(out var shaderDirectory),
                Is.True,
                "SPIR-V shader directory was not found");
            Assert.That(
                VulkanRTPlugin.InitializeDepthPipeline(shaderDirectory),
                Is.True,
                "Depth pipeline initialization failed");

            var renderEventFunc = VulkanRTPlugin.RenderEventFunc;
            Assert.That(renderEventFunc, Is.Not.EqualTo(IntPtr.Zero), "native render event function unavailable");

            Assert.That(UploadTriangleAtZ(NearMeshId, 2.0f), Is.True, "near mesh upload failed");
            Assert.That(UploadTriangleAtZ(FarMeshId, 5.0f), Is.True, "far mesh upload failed");

            var descriptor = new RenderTextureDescriptor(Width, Height, RenderTextureFormat.RFloat, 0)
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
                name = "CLOiSim Vulkan RT Scene Test Output",
                filterMode = FilterMode.Point,
                wrapMode = TextureWrapMode.Clamp,
            };

            try
            {
                Assert.That(output.Create(), Is.True, "RenderTexture creation failed");
                var nativeTexture = output.GetNativeTexturePtr();
                Assert.That(nativeTexture, Is.Not.EqualTo(IntPtr.Zero));

                var depthOutput = new DepthOutput
                {
                    nativeTexture = nativeTexture,
                    width = Width,
                    height = Height,
                    reserved = 0,
                };

                // 1) Only the near mesh instanced -> center depth ~= 2.
                Assert.That(SetSingleInstance(NearMeshId, 0f), Is.True);
                yield return BuildAndTrace(renderEventFunc, depthOutput);
                float nearDepth = 0f;
                yield return ReadCenterDepth(output, v => nearDepth = v);
                Assert.That(nearDepth, Is.InRange(1.5f, 3.0f), $"expected ~2, got {nearDepth}");

                // 2) Swap to the far mesh -> BLAS cache reused for NearMeshId's
                // BLAS (untouched), TLAS rebuilt to reference FarMeshId's BLAS
                // instead -> center depth ~= 5.
                Assert.That(SetSingleInstance(FarMeshId, 0f), Is.True);
                yield return BuildAndTrace(renderEventFunc, depthOutput);
                float farDepth = 0f;
                yield return ReadCenterDepth(output, v => farDepth = v);
                Assert.That(farDepth, Is.InRange(4.5f, 6.0f), $"expected ~5, got {farDepth}");

                // 3) Same meshId as (1), but translated +3 in Z via the
                // instance transform -> proves instance-transform "move"
                // updates are reflected by the next TLAS rebuild, independent
                // of the mesh's own (unchanged) local geometry.
                Assert.That(SetSingleInstance(NearMeshId, 3f), Is.True);
                yield return BuildAndTrace(renderEventFunc, depthOutput);
                float movedDepth = 0f;
                yield return ReadCenterDepth(output, v => movedDepth = v);
                Assert.That(movedDepth, Is.InRange(4.5f, 6.0f), $"expected ~5, got {movedDepth}");
            }
            finally
            {
                output.Release();
                UnityEngine.Object.Destroy(output);
            }
        }
    }
}
