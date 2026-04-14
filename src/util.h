#pragma once

#include <fstream>
#include <ranges>
#include <unordered_set>
#include <vector>

namespace Mirael
{

struct TransparentStringHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept { return std::hash<std::string_view>{}(sv); }
};

using StringSet = std::unordered_set<std::string, TransparentStringHash, std::equal_to<>>;

template <std::ranges::input_range R1, std::ranges::input_range R2>
auto findFirstMissingString(R1 &&required, R2 &&available) -> std::ranges::borrowed_iterator_t<R1>
    requires std::convertible_to<std::ranges::range_value_t<R1>, std::string_view> &&
             std::convertible_to<std::ranges::range_value_t<R2>, std::string_view>
{
    StringSet availableSet;
    for (const auto &a : available)
        availableSet.emplace(std::string_view(a));
    return std::ranges::find_if(required, [&](std::string_view s) { return !availableSet.contains(s); });
}

template <std::ranges::input_range R1, std::ranges::input_range R2>
bool areAllRequiredStringsPresent(R1 &&required, R2 &&available)
    requires std::convertible_to<std::ranges::range_value_t<R1>, std::string_view> &&
             std::convertible_to<std::ranges::range_value_t<R2>, std::string_view>
{
    return required.end() == findFirstMissingString(required, available);
}

std::vector<char> readSmallFile(const std::string &filepath) // intended for reading files < 100 MB
{
    std::ifstream file(filepath, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error(std::format("Failed to open file: {}", filepath));
    }

    const auto rawSize = file.tellg();
    if (rawSize < 0) {
        throw std::runtime_error(std::format("Could not determine size of file: {}", filepath));
    }

    const size_t fileSize                = static_cast<size_t>(rawSize);
    constexpr size_t fileSizeSanityLimit = 100 * 1024 * 1024; // 100 MB
    if (fileSize > fileSizeSanityLimit) {
        throw std::runtime_error(std::format("File \"{}\" is too large at {} bytes.", filepath, fileSize));
    }

    std::vector<char> buffer(fileSize);
    if (fileSize > 0) {
        file.seekg(0, std::ios::beg);
        if (!file.read(buffer.data(), static_cast<std::streamsize>(fileSize))) {
            throw std::runtime_error(std::format("Failed to read file: {}", filepath));
        }
    }

    return buffer;
}

glm::mat3 calculateMappingTransform(glm::vec2 srcStart, glm::vec2 srcEnd, glm::vec2 dstStart, glm::vec2 dstEnd, bool mirrored);

}; // namespace Mirael