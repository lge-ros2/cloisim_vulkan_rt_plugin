using System.Collections;
using NUnit.Framework;
using UnityEngine;
using UnityEngine.TestTools;

namespace CLOiSim.VulkanRT.Tests
{
  public sealed class VulkanRTSmokeTests
  {
    [UnityTest]
    public IEnumerator RenderTextureDepthSmoke()
    {
      var go = new GameObject("VulkanRTSmokeTest");
      var runner = go.AddComponent<VulkanRTSmokeTest>();
      bool? result = null;
      string detail = "";
      yield return runner.Run((ok, message) =>
      {
        result = ok;
        detail = message;
      });
      Object.Destroy(go);
      Assert.That(result, Is.True, detail);
    }
  }
}
