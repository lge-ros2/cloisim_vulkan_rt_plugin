# CLOiSim Vulkan Ray Tracing

Draft native Vulkan Ray Tracing plugin package for Unity.

## Current Scope

- Unity Native Rendering Plugin entry point
- `IUnityGraphicsVulkanV2` integration
- C ABI for querying Vulkan capabilities
- C# wrapper for render event integration
- Linux x86_64 native plugin build structure

## Build

```bash
cmake -S Native~ -B Native~/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build Native~/build -j
```

Copy the build output to the following location.

```text
Runtime/Plugins/Linux/x86_64/libcloisim_vulkan_rt.so
```

## CLOiSim Local Reference

Add to CLOiSim's `Packages/manifest.json`.

```json
{
  "dependencies": {
    "com.lge.cloisim.vulkan-rt": "file:../../cloisim_vulkan_rt_plugin"
  }
}
```
