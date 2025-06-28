#pragma once

#include "color.hpp"
#include "glm.hpp"
#include "imgui.h"
#include "vec2.hpp"
#include <functional>
#include <random>
#include <utility>
#include <vector>

#define unused(...) (void)(unused_helper(__VA_ARGS__))
template <typename Ts> inline void unused_helper(Ts &&...) noexcept {}

struct Vec2Traits {
    std::size_t operator()(const glm::ivec2 &v) const noexcept
    {
        std::size_t h1 = v.x;
        std::size_t h2 = v.y;
        return h1 ^ (h2 << 1);
    }

    bool operator()(const glm::ivec2 &a, const glm::ivec2 &b) const noexcept { return a.x == b.x && a.y == b.y; }
};

struct PairHash {
    std::size_t operator()(const std::pair<int, int> &p) const noexcept
    {
        return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
    }
};

template <typename T> T RemoveRandomElement(std::vector<T> &v, std::mt19937 &rng)
{
    std::uniform_int_distribution<size_t> distribution(0, v.size() - 1);
    auto index = distribution(rng);
    std::swap(v[index], v.back());
    T value = std::move(v.back());
    v.pop_back();
    return value;
}

float PointDistanceToLineSegment(const glm::vec2 &P, const glm::vec2 &A, const glm::vec2 &B);

inline Color convert(ImVec4 color) { return {color.x, color.y, color.z, color.w}; }

template <typename A, typename B, typename C> struct Triplet {
    A first;
    B second;
    C third;

    constexpr Triplet() = default;
    constexpr Triplet(const A &a) : first(a) {}
    constexpr Triplet(const A &a, const B &b) : first(a), second(b) {}
    constexpr Triplet(const A &a, const B &b, const C &c) : first(a), second(b), third(c) {}

    auto operator<=>(const Triplet &) const = default;
};
