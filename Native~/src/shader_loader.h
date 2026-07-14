#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

class ShaderLoader
{
public:
    static bool LoadSpirv(
        const std::filesystem::path& path,
        std::vector<uint32_t>& words);
};
