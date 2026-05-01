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
#include <warthog/io/fwd.h>

#include <memory_resource>
#include <cstring>
#include <istream>

namespace warthog::manager
{

/// @brief a class for managing a set of patches
///
/// Stores a list of patches at 
class grid_patch_set
{
public:
	enum flags : uint32_t {
		DEFAULT = 0u, ///< default options
		SKIP_HEADER = 1u << 0, ///< will not read header
		FORCE_HEADER = 1u << 1, ///< must read header
		IGNORE_INDEX = 1u << 2, ///< ignores patch read index
	};
	using bittable = domain::gridmap::bittable;
	static constexpr uint16_t npos = (uint16_t)-1u;
	grid_patch_set(std::pmr::memory_resource* upstream = nullptr) :
		grid_res_(upstream != nullptr ? upstream : std::pmr::get_default_resource())
	{ }

	bool load(std::istream& file);
	bool load(const std::filesystem::path& maps);

	/// @brief reads from a serialize, checking for errors.  Can read both type octile and patch files.
	/// @param S the bittable_serialize, must be open
	/// @param max_grids the max grids to read, or -1 for no limit
	/// @param flags flags that control how to read, check enum flags
	/// @return the number of grids read successfully, or <0 for negative index of patch that failed to be read successfully
	///
	/// This function will load from an established bittable_serialize, reading all grids and appending to existing patches.
	/// If type octile, only one grid is read as the first patch, otherwise up to max_grids (default all) are read.
	/// 
	/// By DEFAULT, it will read the header if not already read and append all remaining grids to existing patches, but will error if the index in the file does not match the index in this class.
	/// SKIP_HEADER requires that S have already read the header else errors.
	/// FORCE_HEADER requires that S have not read the header else errors.
	/// IGNORE_INDEX will not error if index in patch file does not match.
	int deserialize(io::bittable_serialize& S, int max_grids = -1, uint32_t flags = DEFAULT);

	bool push_copy(bittable table, uint16_t offset_x = 0, uint16_t offset_y = 0,
		uint16_t width = npos, uint16_t height = npos);
	
	bool push_ref(bittable patch);

	void reset();

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
