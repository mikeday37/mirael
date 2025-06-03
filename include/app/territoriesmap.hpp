#pragma once

#include <unordered_map>
#include <unordered_set>

#include "mirael_glm_ext.hpp"
#include "vec2.hpp"

class TerritoriesMap {
public:

	void SetCell(int x, int y, int territoryId);
	int GetCell(int x, int y) const;

private:
	std::unordered_map<glm::ivec2, int, Vec2Traits, Vec2Traits> map_; // from cooredinate to which territory claimed it, or 0 if none
	std::unordered_map<int, std::unordered_set<glm::ivec2, Vec2Traits, Vec2Traits>> territories_; // from territory id to the cells it's claimed
	glm::ivec2 extentMin_; // the minimum x and y coords of claimed cells
	glm::ivec2 extentMax_; // the maximum x and y coords of claimed cells
};
