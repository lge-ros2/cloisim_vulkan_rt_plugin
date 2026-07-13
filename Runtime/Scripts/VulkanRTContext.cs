using System;
using UnityEngine;
using UnityEngine.Rendering;

namespace CLOiSim.VulkanRT
{
    public sealed class VulkanRTContext : IDisposable
    {
        public enum RenderEvent
        {
            ProbeCapabilities = 1,
            BuildScene = 2,
            TraceSensor = 3,
            ReleaseDeferred = 4
        }

        private readonly CommandBuffer commandBuffer;
        private readonly IntPtr renderEventFunc;

        public VulkanRTContext(string name = "CLOiSim Vulkan RT")
        {
            if (!VulkanRTPlugin.IsVulkan)
                throw new NotSupportedException("Vulkan graphics API is required.");

            renderEventFunc = VulkanRTPlugin.GetRenderEventFunc();
            if (renderEventFunc == IntPtr.Zero)
                throw new InvalidOperationException("Native render event function is unavailable.");

            commandBuffer = new CommandBuffer { name = name };
        }

        public CommandBuffer CommandBuffer => commandBuffer;

        public void Issue(RenderEvent renderEvent, IntPtr data = default)
        {
            commandBuffer.IssuePluginEventAndData(
                renderEventFunc,
                (int)renderEvent,
                data);
        }

        public void Dispose()
        {
            commandBuffer?.Release();
        }
    }
}
