using System;
using System.Collections;
using System.IO;
using UnityEngine;
using UnityEngine.Rendering;

namespace CLOiSim.VulkanRT.Samples
{
    public sealed class MinimalDepthSensorSample : MonoBehaviour
    {
        private const string ShaderDirectoryEnvironment =
            "CLOISIM_RT_SHADER_DIR";

        private const string PackageId =
            "com.lge-ros2.cloisim.vulkan-rt";

        [SerializeField]
        private bool runOnStart = true;

        [SerializeField]
        private string shaderDirectoryOverride = string.Empty;

        public bool? LastSucceeded { get; private set; }
        public string LastMessage { get; private set; } = string.Empty;

        private bool running;

        private IEnumerator Start()
        {
            if (runOnStart)
                yield return RunSmokeTest();
        }

        [ContextMenu("Run Minimal Depth Sensor Smoke Test")]
        private void RunFromInspector()
        {
            if (!running)
                StartCoroutine(RunSmokeTest());
        }

        public IEnumerator RunSmokeTest()
        {
            if (running)
                yield break;

            running = true;
            LastSucceeded = null;
            LastMessage = string.Empty;

            try
            {
                if (SystemInfo.graphicsDeviceType != GraphicsDeviceType.Vulkan)
                {
                    Complete(false, "Vulkan graphics API is required.");
                    yield break;
                }

                if (!TryResolveShaderDirectory(out var shaderDirectory))
                {
                    Complete(
                        false,
                        "SPIR-V directory was not found. Set Shader Directory " +
                        "Override or CLOISIM_RT_SHADER_DIR.");
                    yield break;
                }

                if (!VulkanRTPlugin.InitializeDepthPipeline(shaderDirectory))
                {
                    Complete(false, "Depth pipeline initialization failed.");
                    yield break;
                }

                var smokeTest = GetComponent<VulkanRTSmokeTest>();
                if (smokeTest == null)
                    smokeTest = gameObject.AddComponent<VulkanRTSmokeTest>();

                var callbackInvoked = false;
                var succeeded = false;
                var detail = string.Empty;

                yield return smokeTest.Run((ok, message) =>
                {
                    callbackInvoked = true;
                    succeeded = ok;
                    detail = message ?? string.Empty;
                });

                if (!callbackInvoked)
                {
                    Complete(false, "Smoke test returned no result.");
                    yield break;
                }

                Complete(succeeded, detail);
            }
            catch (Exception exception)
            {
                Complete(false, exception.ToString());
            }
            finally
            {
                running = false;
            }
        }

        private bool TryResolveShaderDirectory(out string directory)
        {
            directory = shaderDirectoryOverride?.Trim() ?? string.Empty;

            if (string.IsNullOrEmpty(directory))
            {
                directory = Environment.GetEnvironmentVariable(
                    ShaderDirectoryEnvironment) ?? string.Empty;
            }

            if (string.IsNullOrEmpty(directory))
            {
                directory = Path.GetFullPath(
                    Path.Combine(
                        Application.dataPath,
                        "..",
                        "Packages",
                        PackageId,
                        "Runtime",
                        "Shaders"));
            }

            if (!Directory.Exists(directory))
                return false;

            foreach (var name in new[]
                     {
                         "depth.rgen.spv",
                         "depth.rmiss.spv",
                         "depth.rchit.spv",
                     })
            {
                if (!File.Exists(Path.Combine(directory, name)))
                    return false;
            }

            return true;
        }

        private void Complete(bool succeeded, string message)
        {
            LastSucceeded = succeeded;
            LastMessage = message;

            var formatted =
                $"[CLOiSim Vulkan RT] Minimal depth sample: {message}";

            if (succeeded)
                Debug.Log(formatted, this);
            else
                Debug.LogError(formatted, this);
        }
    }
}
