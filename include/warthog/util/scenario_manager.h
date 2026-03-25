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

class scenario_manager
{
public:
	scenario_manager();
	~scenario_manager();

	experiment*
	get_experiment(unsigned int which)
	{
		if(which < experiments_.size()) { return experiments_[which]; }
		return 0;
	}
	const experiment*
	get_experiment(unsigned int which) const
	{
		if(which < experiments_.size()) { return experiments_[which]; }
		return 0;
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
	clear()
	{
		experiments_.clear();
		experiments_res_.release();
	}

	void
	generate_experiments(domain::gridmap*, int num);
	void
	load_scenario(const std::filesystem::path& filelocation);
	void
	load_scenario(
	    std::istream& file, std::filesystem::path&& mapfile_override = {});
	void
	write_scenario(std::ostream& out);
	void
	sort(); // organise by increasing solution length

protected:
	std::errc
	load_gppc_scenario(std::istream& scenfile);

	std::pmr::monotonic_buffer_resource experiments_res_;
	std::vector<experiment*> experiments_;
	std::filesystem::path sfile_;
	std::filesystem::path mfile_;
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
