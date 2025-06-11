#pragma once

#include "vec2.hpp"
#include <utility>
#include <functional>

struct Vec2Traits {
	std::size_t operator()(const glm::ivec2 &v) const {
		std::size_t h1 = v.x;
		std::size_t h2 = v.y;
		return h1 ^ (h2 << 1);
	}

	bool operator()(const glm::ivec2 &a, const glm::ivec2 &b) const {
		return a.x == b.x && a.y == b.y;
	}
};

struct PairHash {
	std::size_t operator()(const std::pair<int, int> &p) const {
		return std::hash<int>{}(p.first) ^ (std::hash<int>{}(p.second) << 1);
	}
};
