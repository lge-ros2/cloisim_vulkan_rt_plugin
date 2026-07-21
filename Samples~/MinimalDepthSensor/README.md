# Minimal Depth Sensor

This sample validates the Unity Vulkan RenderTexture integration against the
plugin-owned triangle scene.

## Run

1. Import the sample from Package Manager.
2. Open `Scenes/MinimalDepthSensor.unity`.
3. Start Play Mode.
4. Check Console for `[CLOiSim Vulkan RT] Minimal depth sample`.

Expected output:

- center depth is between 1.5 and 3.0
- corner depth is greater than 1.0e20

Shader lookup order:

1. `Shader Directory Override` in the component
2. `CLOISIM_RT_SHADER_DIR`
3. `Packages/com.lge-ros2.cloisim.vulkan-rt/Runtime/Shaders`
