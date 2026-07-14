#include "shader_loader.h"

#include <fstream>
#include <limits>

bool ShaderLoader::LoadSpirv(
    const std::filesystem::path& path,
    std::vector<uint32_t>& words)
{
    words.clear();

    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return false;

    const std::streamsize byteCount = stream.tellg();
    if (byteCount <= 0 ||
        byteCount % static_cast<std::streamsize>(sizeof(uint32_t)) != 0)
    {
        return false;
    }

    const auto wordCount =
        static_cast<std::uintmax_t>(byteCount) / sizeof(uint32_t);
    if (wordCount > std::numeric_limits<std::size_t>::max())
        return false;

    words.resize(static_cast<std::size_t>(wordCount));
    stream.seekg(0, std::ios::beg);

    if (!stream.read(
            reinterpret_cast<char*>(words.data()),
            byteCount))
    {
        words.clear();
        return false;
    }

    constexpr uint32_t kSpirvMagic = 0x07230203U;
    if (words.empty() || words.front() != kSpirvMagic)
    {
        words.clear();
        return false;
    }

    return true;
}
