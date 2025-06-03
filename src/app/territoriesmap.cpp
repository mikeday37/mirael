#include "app_pch.hpp"

#include "app/territoriesmap.hpp"

#include "vec2.hpp"

void TerritoriesMap::SetCell(int x, int y, int territoryId)
{
	glm::ivec2 v(x, y);
	bool wasEmpty = map_.empty();

	// if we're claiming a cell
	if (territoryId) {

		// insert to the map and see if it really got inserted
		auto [_, inserted] = map_.insert({v, territoryId});

		// if it didn't, it was already there, and we don't need to do anything else
		if (!inserted) {
			return;
		}

		// otherwise, we have to add it to the cell set for the territory, too
		territories_[territoryId].insert(v);

		// if the map was empty, we set both extents to v, otherwise, extend as needed
		if (wasEmpty) {
			extentMin_ = extentMax_ = v;
		} else {
			if (v.x < extentMin_.x) extentMin_.x = v.x;
			if (v.x > extentMax_.x) extentMax_.x = v.x;
			if (v.y < extentMin_.y) extentMin_.y = v.y;
			if (v.y > extentMax_.y) extentMax_.y = v.y;
		}
	}
	else {
		// if empty, we don't need to do a thing
		if (wasEmpty) {
			return;
		}

		// we're clearing a cell, so attempt to remove it from the map
		auto removed = map_.erase(v);

		// we don't recalculate extent on removal, but will reset it when empty
		if (map_.empty()) {
			extentMin_ = extentMax_ = glm::ivec2(0, 0);
		}

		// if it wasn't already there, we don't have to do anything else
		if (!removed) {
			return;
		}

		// otherwise, we have to remove it from the cell set for the territory, too
		territories_[territoryId].erase(v);
	}
}

int TerritoriesMap::GetCell(int x, int y) const
{
    if (auto c = map_.find({x, y}); c != map_.end())
		return c->second;
	else
		return 0;
}
