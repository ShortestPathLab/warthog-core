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
		if(which >= experiments_.size()) {
			WARTHOG_GWARN_FMT("scenario has max {} experiments, cannot retrive {}", experiments_.size(), which);
			return nullptr;
		}
		return experiments_[which];
	}
	const experiment*
	get_experiment(uint32_t which) const
	{
		if(which >= experiments_.size()) {
			WARTHOG_GWARN_FMT("scenario has max {} experiments, cannot retrive {}", experiments_.size(), which);
			return nullptr;
		}
		return experiments_[which];
	}

	std::span<experiment* const>
	get_experiments() noexcept
	{
		return experiments_;
	}
	std::span<const experiment* const>
	get_experiments() const noexcept
	{
		return experiments_;
	}

	std::span<scenario_command>
	get_commands() noexcept
	{
		return commands_;
	}
	std::span<const scenario_command>
	get_commands() const noexcept
	{
		return commands_;
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
	last_file_loaded() const noexcept
	{
		return sfile_.string();
	}
	const std::filesystem::path&
	scenario_filename() const noexcept
	{
		return sfile_;
	}
	const std::filesystem::path&
	map_filename() const noexcept
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

	std::string_view get_cost_type() const noexcept
	{
		return cost_type_;
	}
	void set_cost_type(std::string_view v) noexcept
	{
		cost_type_ = v;
	}
	void set_cost_type(io::cost_type c) noexcept
	{
		cost_type_ = io::scenario_serialize::get_dist_str(c);
	}

	bool is_static_scenario() const noexcept { return static_scenario_start_ >= 0; }
	int32_t get_static_scenario_start() const noexcept { return static_scenario_start_; }

	uint32_t get_scenario_width() const noexcept
	{
		return scenario_width_;
	}
	void set_scenario_width(uint32_t width) noexcept
	{
		scenario_width_ = width;
	}

	uint32_t get_scenario_height() const noexcept
	{
		return scenario_height_;
	}
	void set_scenario_height(uint32_t height) noexcept
	{
		scenario_height_ = height;
	}

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
	std::string cost_type_;
	io::scenario_version version_ = io::scenario_version::UNKNOWN;
	uint32_t scenario_width_ = 0;
	uint32_t scenario_height_ = 0;
	uint32_t query_count_ = 0;
	uint32_t patch_count_ = 0;
	int32_t static_scenario_start_ = -1; ///< >=0: is static scenario where query commands start at pos, else is dynamic scenario
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
