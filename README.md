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

## Test

CPU-only unit tests (ABI layout, shader loader, SPIR-V outputs, Vulkan instance layout) run via CTest.

```bash
cmake -S Native~ -B Native~/build -DCMAKE_BUILD_TYPE=RelWithDebInfo -DBUILD_TESTING=ON
cmake --build Native~/build -j
ctest --test-dir Native~/build --output-on-failure
```

To also build GPU tests (require a real Vulkan device; skip with exit code 77 otherwise), add `-DCLOISIM_RT_BUILD_GPU_TESTS=ON`.

.NET native-interop tests load the built plugin and exercise the C# wrapper.

```bash
CLOISIM_RT_PLUGIN_PATH=Native~/build/libcloisim_vulkan_rt.so \
  dotnet run --project Tests~/DotNet/CLOiSim.VulkanRT.NativeTests.csproj --configuration Release
```

Or run the full pipeline (build, CTest, .NET tests) in one step:

```bash
./scripts/run_native_validation.sh
```

## CLOiSim Local Reference

Add to CLOiSim's `Packages/manifest.json`.

```json
{
  "dependencies": {
    "com.lge-ros2.cloisim.vulkan-rt": "file:../../cloisim_vulkan_rt_plugin"
  }
}
```
