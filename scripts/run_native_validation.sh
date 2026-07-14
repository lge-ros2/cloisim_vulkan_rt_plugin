#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${root}/Native~/build"

cmake \
  -S "${root}/Native~" \
  -B "${build_dir}" \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DBUILD_TESTING=ON \
  -DCLOISIM_RT_BUILD_GPU_TESTS=ON

cmake --build "${build_dir}" -j"$(nproc)"
ctest --test-dir "${build_dir}" --output-on-failure

plugin="${build_dir}/libcloisim_vulkan_rt.so"
test -s "${plugin}"

CLOISIM_RT_PLUGIN_PATH="${plugin}" \
  dotnet run \
    --project "${root}/Tests~/DotNet/CLOiSim.VulkanRT.NativeTests.csproj" \
    --configuration Release
