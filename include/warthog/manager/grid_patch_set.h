#ifndef WARTHOG_MANAGER_GRID_PATCH_SET_H
#define WARTHOG_MANAGER_GRID_PATCH_SET_H

/// @file grid_patch_set.h
///
/// Utility to store grid map/patches.
/// Can read with some flexibility and be used with scenario_runner to update a gridmap.
///
/// @author: Ryan Hechenberger
/// @created: 2026-04-10
///

#include <warthog/domain/gridmap.h>
#include <warthog/io/fwd.h>
#include <warthog/memory/bittable.h>

#include <cstring>
#include <istream>
#include <memory_resource>

namespace warthog::manager
{

/// @brief a class for managing a set of patches
///
/// Stores a list of patches in a vector.
/// Supports loading of patches from a file/stream with flags to control this.
/// User can also add their own patches.
class grid_patch_set
{
public:
	enum flags : uint32_t
	{
		DEFAULT      = 0u,      ///< default options
		SKIP_HEADER  = 1u << 0, ///< will not read header
		FORCE_HEADER = 1u << 1, ///< must read header
		IGNORE_INDEX = 1u << 2, ///< ignores patch read index
	};
	using bittable                 = domain::gridmap::bittable;
	static constexpr uint16_t npos = (uint16_t)-1u;

	grid_patch_set(std::pmr::memory_resource* upstream = nullptr)
	    : grid_res_(
	          upstream != nullptr ? upstream
	                              : std::pmr::get_default_resource())
	{ }

	/// @brief read istream as whole patch set
	/// @param file open text istream
	/// @param max_grids maximum number of grids to read
	/// @return true if success, false otherwise
	bool
	load(std::istream& file, int max_grids = -1);

	/// @brief opens file and read as whole patch set
	/// @param maps the filename to open
	/// @param max_grids maximum number of grids to read
	/// @return true if success, false otherwise
	bool
	load(const std::filesystem::path& maps, int max_grids = -1);

	/// @brief reads from a serialize, checking for errors.  Can read both type
	/// octile and patch files.
	/// @param S the bittable_serialize, must be open
	/// @param max_grids the max grids to read, or -1 for no limit
	/// @param flags flags that control how to read, check enum flags
	/// @return the number of grids read successfully, or <0 for negative index
	/// of patch that failed to be read successfully
	///
	/// This function will load from an established bittable_serialize, reading
	/// all grids and appending to existing patches. If type octile, only one
	/// grid is read as the first patch, otherwise up to max_grids (default
	/// all) are read.
	///
	/// By DEFAULT, it will read the header if not already read and append all
	/// remaining grids to existing patches, but will error if the index in the
	/// file does not match the index in this class. SKIP_HEADER requires that
	/// S have already read the header else errors. FORCE_HEADER requires that
	/// S have not read the header else errors. IGNORE_INDEX will not error if
	/// index in patch file does not match.
	int
	deserialize(
	    io::bittable_serialize& S, int max_grids = -1,
	    uint32_t flags = DEFAULT);

	/// @brief copies a user-provided bittable, subregion from offset with width/height
	/// @param table the base table to push
	/// @return true on success, false otherwise
	/// @pre (offset_x == 0 && offset_y == 0 && width == npos && height == npos) || (width != npos && height != npos)
	///
	/// Will copy from (offset_x,offset_y) table of width by height.
	/// Subtable must fully fit within table or fail.
	/// Defaults will copy whole bittable, if any arguments are changed then width/height
	/// must be specified (i.e. cannot be npos).
	bool
	push_copy(
	    bittable table, uint16_t offset_x = 0, uint16_t offset_y = 0,
	    uint16_t width = npos, uint16_t height = npos);

	/// @brief pushes a bittable to set, not copying contents, does not own table memory
	/// @param patch the patch to push
	/// @return true on success, false otherwise
	bool
	push_ref(bittable patch);

	void
	reset();

	size_t
	size() const noexcept
	{
		return patches_.size();
	}
	bittable
	get_patch(size_t i) const
	{
		return patches_.at(i);
	}
	std::span<const bittable>
	get_patches() const noexcept
	{
		return patches_;
	}

protected:
	std::pmr::monotonic_buffer_resource grid_res_;
	std::vector<bittable> patches_;
};

} // namespace warthog::manager

#endif // WARTHOG_MANAGER_PATCH_SET_H
