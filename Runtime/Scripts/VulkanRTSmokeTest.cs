using System;
using System.Collections;
using System.Runtime.InteropServices;
using UnityEngine;
using UnityEngine.Rendering;
namespace CLOiSim.VulkanRT {
 public sealed class VulkanRTSmokeTest : MonoBehaviour {
  [StructLayout(LayoutKind.Sequential)] private struct DepthOutput { public IntPtr nativeTexture; public uint width,height,reserved; }
  private const string LibraryName="cloisim_vulkan_rt";
  [DllImport(LibraryName)] private static extern int CLOISimRt_SetDepthOutput(ref DepthOutput output);
  [DllImport(LibraryName)] private static extern int CLOISimRt_IsSmokeSceneReady();
  [DllImport(LibraryName)] private static extern int CLOISimRt_GetLastTraceStatus();
  public IEnumerator Run(Action<bool,string> completed) {
   if(SystemInfo.graphicsDeviceType!=GraphicsDeviceType.Vulkan){completed(false,"Vulkan backend required");yield break;}
   var descriptor=new RenderTextureDescriptor(16,16,RenderTextureFormat.RFloat,0){enableRandomWrite=true,msaaSamples=1,volumeDepth=1,dimension=TextureDimension.Tex2D};
   using var output=new RenderTexture(descriptor); output.Create();
   var request=new DepthOutput{nativeTexture=output.GetNativeTexturePtr(),width=16,height=16};
   if(CLOISimRt_SetDepthOutput(ref request)!=0){completed(false,"SetDepthOutput failed");yield break;}
   GL.IssuePluginEvent(VulkanRTPlugin.RenderEventFunc,2); yield return new WaitForEndOfFrame();
   GL.IssuePluginEvent(VulkanRTPlugin.RenderEventFunc,3); yield return new WaitForEndOfFrame();
   if(CLOISimRt_IsSmokeSceneReady()!=1||CLOISimRt_GetLastTraceStatus()!=1){completed(false,"native smoke trace failed");yield break;}
   var readback=AsyncGPUReadback.Request(output,0,TextureFormat.RFloat);while(!readback.done)yield return null;
   if(readback.hasError){completed(false,"GPU readback failed");yield break;}
   var data=readback.GetData<float>();float center=data[8*16+8],corner=data[0];bool ok=float.IsFinite(center)&&center>1.5f&&center<3.0f&&corner>1.0e20f;
   completed(ok,$"center={center}, corner={corner}");
  }
 }
}
