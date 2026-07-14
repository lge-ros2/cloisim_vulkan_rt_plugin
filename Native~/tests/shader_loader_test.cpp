#include "shader_loader.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace
{
void WriteWords(
    const std::filesystem::path& path,
    const std::vector<uint32_t>& words)
{
    std::ofstream stream(path, std::ios::binary);
    stream.write(
        reinterpret_cast<const char*>(words.data()),
        static_cast<std::streamsize>(words.size() * sizeof(uint32_t)));
}

int Fail(const char* message)
{
    std::cerr << "[FAIL] " << message << '\n';
    return 1;
}
}

int main()
{
    const auto directory = std::filesystem::temp_directory_path() /
        "cloisim_vulkan_rt_shader_loader_test";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    const auto validPath = directory / "valid.spv";
    const auto invalidMagicPath = directory / "invalid_magic.spv";
    const auto invalidSizePath = directory / "invalid_size.spv";

    WriteWords(validPath, {0x07230203U, 0x00010000U, 0U, 1U, 0U});
    WriteWords(invalidMagicPath, {0xDEADBEEFU, 0U});

    {
        std::ofstream stream(invalidSizePath, std::ios::binary);
        const char bytes[3] = {1, 2, 3};
        stream.write(bytes, sizeof(bytes));
    }

    std::vector<uint32_t> words;
    if (!ShaderLoader::LoadSpirv(validPath, words))
        return Fail("valid SPIR-V was rejected");
    if (words.size() != 5 || words.front() != 0x07230203U)
        return Fail("valid SPIR-V content mismatch");
    if (ShaderLoader::LoadSpirv(invalidMagicPath, words))
        return Fail("invalid magic was accepted");
    if (ShaderLoader::LoadSpirv(invalidSizePath, words))
        return Fail("unaligned byte length was accepted");
    if (ShaderLoader::LoadSpirv(directory / "missing.spv", words))
        return Fail("missing file was accepted");

    std::filesystem::remove_all(directory);
    std::cout << "[OK] shader loader validation passed\n";
    return 0;
}
