#ifndef WARTHOG_MANAGER_GRID_PATCH_SET_H
#define WARTHOG_MANAGER_GRID_PATCH_SET_H

// scenario_runner.h
//
// Take a scenario_manager object and be able to progress through a dynamic scenario.
//
// @author: Ryan Hechenberger
// @created: 2026-04-10
//

#include <warthog/memory/bittable.h>
#include <warthog/domain/gridmap.h>

#include <memory_resource>
#include <cstring>

namespace warthog::manager
{

/// @brief a class for managing a set of patches
///
/// Stores a list of patches at 
class grid_patch_set
{
public:
	using bittable = domain::gridmap::bittable;
	static constexpr uint16_t npos = (uint16_t)-1u;
	grid_patch_set(std::pmr::memory_resource* upstream = nullptr) :
		grid_res_(1024*4, upstream)
	{ }

	bool push_copy(bittable table, uint16_t offset_x = 0, uint16_t offset_y = 0,
		uint16_t width = npos, uint16_t height = npos)
	{
		if (table.size() == 0) {
			return false;
		}
		bittable patch;
		if (offset_x == 0 && offset_y == 0 && width == npos && height == npos) {
			// copy as is
			auto size = table.size_bytes();
			auto* grid_data = static_cast<bittable::value_type*>(grid_res_.allocate(size, alignof(bittable::value_type)));
			std::memcpy(grid_data, table.data(), size);
			patch = bittable(grid_data, table.width(), table.height());
		} else {
			// crop
			if (offset_x >= table.width() || width > table.width() || offset_x + width > table.width())
			{
				return false;
			}
			if (offset_y >= table.height() || height > table.height() || offset_y + height > table.height())
			{
				return false;
			}
			auto size = bittable::calc_array_size(width, height);
			auto* grid_data = static_cast<bittable::value_type*>(grid_res_.allocate(size, alignof(bittable::value_type)));
			patch = bittable(grid_data, table.width(), table.height());
			table.copy(patch, pad_id::zero(), table.xy_to_id(offset_x, offset_y), width, height);
		}
		patches_.push_back(patch);
	}
	bool push_ref(bittable patch)
	{
		patches_.push_back(patch);
		return true;
	}

	void reset()
	{
		patches_.clear();
		grid_res_.release();
	}

	size_t size() const noexcept
	{
		return patches_.size();
	}
	bittable get_patch(size_t i) const
	{
		return patches_.at(i);
	}
	std::span<const bittable> get_patches() const noexcept
	{
		return patches_;
	}

protected:
	std::pmr::monotonic_buffer_resource grid_res_;
	std::vector<bittable> patches_;
};

} // namespace warthog::manager

#endif // WARTHOG_MANAGER_PATCH_SET_H
