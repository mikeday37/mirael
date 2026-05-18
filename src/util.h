#pragma once

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

std::vector<char> readSmallFile(const std::string &filepath); // intended for small files, 100 MB max

glm::mat3 calculateMappingTransform(glm::vec2 srcStart, glm::vec2 srcEnd, glm::vec2 dstStart, glm::vec2 dstEnd, bool mirrored);

} // namespace Mirael