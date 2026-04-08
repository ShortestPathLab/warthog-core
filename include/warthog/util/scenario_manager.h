#ifndef WARTHOG_UTIL_SCENARIO_MANAGER_H
#define WARTHOG_UTIL_SCENARIO_MANAGER_H

// scenario_manager.h
//
// Read and generate/write scenario files
//
//	Supported formats for read:
//	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
//		  (fields: bucket,map,mapwidth,mapheight,sx,sy,gx,gy,distance)
//	    - DIMACS format (as at the 9th DIMACS Implementation Challenge)
//	      (fields: q [source-id] [target-id])
//
//	Supported formats for generate/write:
//	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
//
// @author: dharabor
// @created: 21/08/2012
//

#include "experiment.h"
#include <warthog/domain/gridmap.h>
#include <warthog/io/grid.h>
#include <warthog/io/scenario.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory_resource>
#include <vector>

namespace warthog::util
{

struct scenario_command
{
	enum type_ : uint8_t {
		SNAPSHOT,
		PATCH,
		QUERY
	};
	int type; ///< command type
	int32_t bucket; ///< bucket id number (meta), snapshot id for dynamic
	uint32_t id; ///< SNAPSHOT: snapshot num, PATCH: patch to apply, QUERY: query id
	union cmd_ {
		struct snapshot_ { } snapshot;
		struct patch_ {
			uint16_t topleft_x;
			uint16_t topleft_y;
		} patch;
		struct query_ {
			uint32_t experiment_id; ///< experiment number
		} query;
	} cmd; ///< command union based on type

	static constexpr scenario_command make_snapshot(int32_t bucket_id, uint32_t snapshot_id) noexcept
	{
		return scenario_command{SNAPSHOT, bucket_id, snapshot_id, {.snapshot={}}};
	}
	static constexpr scenario_command make_patch(int32_t bucket_id, uint32_t patch_id, uint16_t x, uint16_t y) noexcept
	{
		return scenario_command{PATCH, bucket_id, patch_id, {.patch={x,y}}};
	}
	static constexpr scenario_command make_query(int32_t bucket_id, uint32_t query_id, uint32_t experiment_id) noexcept
	{
		return scenario_command{QUERY, bucket_id, query_id, {.query={experiment_id}}};
	}
};

class scenario_manager
{
public:
	scenario_manager();
	~scenario_manager();

	experiment*
	get_experiment(uint32_t which)
	{
		if(which >= experiments_.size())
			return nullptr;
		if (static_scenario_start_ >= 0) {
			return experiments_[which];
		}
		// handle dynamic scenario
		if (experiment_at_ == which) {
			return experiments_[which];
		} else if (experiment_at_ < (int32_t)which) {
			return experiment_next((int32_t)which - experiment_at_).first;
		} else {
			throw std::logic_error("scenario_manager::get_experiment can only progress forwards in dynamic scenarios.");
		}
	}
	const experiment*
	get_experiment(uint32_t which) const
	{
		if (static_scenario_start_ >= 0) {
			if(which < experiments_.size()) { return experiments_[which]; }
			return nullptr;
		}
		throw std::logic_error("scenario_manager::get_experiment can only be const for static scenarios.");
	}

	/// @brief progress from current to count experiment away and return it
	/// @param count 
	/// @return pair of the reached query and snapshot id.
	///
	/// Will progress through commands until count queries are encounted, returning the final query.
	/// When count == 1, is exactly the next query.
	/// All patches required 
	std::pair<experiment*, int>
	experiment_next(uint32_t count = 1);

	/// @brief reset scenario to first command
	void
	restart();
	/// @brief goto the start of the next snapshot (SNAPSHOT command)
	/// @return snapshot id reached, or -1 if at end of commands (no more snapshots)
	///
	/// If current command is SNAPSHOT, if id != current_snapshot() then already at next snapshot,
	/// otherwise progress until next SNAPSHOT command is reached (or end of commands).
	/// Clears all patches from get_patches() and replaces with any PATCH command to next snapshot.
	int
	snapshot_next(bool clear_patch = true);
	/// @brief starting at SNAPSHOT or current PATCH, apply all patches until reaching SNAPSHOT or QUERY
	/// @param clear_patch clears get_patches()
	/// @return the number of patches, patch id are retrivable from get_patches()
	///
	/// Requires to be on SNAPSHOT or PATCH, otherwise returns 0 and does nothing.
	/// If @clear_patch is set, clears get_patches().
	/// Appends all processed PATCH commands to get_patches().
	int
	snapshot_patches(bool clear_patch = true);
	/// @brief returns the current query experiment if at query and progress to next command
	/// @return the current query experiment if command is query, otherwise nullptr
	///
	/// Requires to be on QUERY, otherwise return nullptr and do nothing.
	/// If QUERY, returns corrisponding experiment and goto next command.
	/// Does not affect get_patches().
	experiment*
	snapshot_query();

	bool complete() const noexcept { return command_at_ >= commands_.size(); }
	
	std::span<const uint32_t>
	get_patches() const noexcept
	{
		return patches_;
	}

	void
	add_experiment(experiment* newexp)
	{
		experiments_.push_back(newexp);
	}

	uint32_t
	num_experiments() const noexcept
	{
		return (uint32_t)experiments_.size();
	}

	size_t
	mem() const noexcept
	{
		return sizeof(*this) + sizeof(experiment) * experiments_.size();
	}

	std::string
	last_file_loaded() noexcept
	{
		return sfile_.string();
	}
	const std::filesystem::path&
	scenario_filename() noexcept
	{
		return sfile_;
	}
	const std::filesystem::path&
	map_filename() noexcept
	{
		return mfile_;
	}
	void
	clear();

	void
	load_scenario(const std::filesystem::path& filelocation);
	void
	load_scenario(
	    std::istream& file, std::filesystem::path&& mapfile_override = {});
	void
	write_scenario(std::ostream& out);

	bool is_static_scenario() const noexcept { return static_scenario_start_ >= 0; }

protected:
	std::errc
	load_gppc_scenario(std::istream& scenfile);

	std::errc
	load_gppc_scenario_body_v1(io::scenario_serialize& si);
	std::errc
	load_gppc_scenario_body_v2(io::scenario_serialize& si);

	std::string_view copy_string(std::string_view str);

	std::pmr::monotonic_buffer_resource experiments_res_;
	std::vector<experiment*> experiments_;
	std::vector<scenario_command> commands_;
	std::vector<uint32_t> patches_;
	std::filesystem::path sfile_;
	std::filesystem::path mfile_;
	io::scenario_version version_ = io::scenario_version::UNKNOWN;
	uint32_t query_count_ = 0;
	uint32_t patch_count_ = 0;
	int32_t static_scenario_start_ = -1; ///< >=0: is static scenario where query commands start at pos, else is dynamic scenario
	uint32_t command_at_ = 0; ///< command at, used for dynamic scenario
	int32_t experiment_at_ = 0;
	int32_t snapshot_at_ = -1;
};

std::filesystem::path
find_map_filename(
    const scenario_manager& scenmgr,
    const std::filesystem::path& sfilename = {});

std::filesystem::path
find_map_filename(
    const std::filesystem::path& scenmgr,
    const std::filesystem::path& sfilename = {});

} // namespace warthog::util

#endif // WARTHOG_UTIL_SCENARIO_MANAGER_H
