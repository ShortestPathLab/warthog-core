#include <warthog/scenario/scenario_manager.h>

#include <warthog/io/log.h>
#include <warthog/io/scenario_serialize.h>
#include <warthog/search/problem_instance.h>

#include <cstdlib>
#include <cstring>

namespace warthog::scenario
{

scenario_manager::scenario_manager() { }

scenario_manager::~scenario_manager() = default;

void
scenario_manager::clear()
{
	experiments_.clear();
	experiments_res_.release();
	mfile_.clear();
	inst_count_            = 0;
	patch_count_           = 0;
	snapshot_count_        = 0;
	static_scenario_start_ = -1;
}

void
scenario_manager::set_cost_type(io::cost_type c) noexcept
{
	cost_type_ = io::scenario_serialize::get_cost_str(c);
}

void
scenario_manager::load_scenario(const std::filesystem::path& filelocation)
{
	sfile_ = filelocation;
	std::ifstream in(filelocation);
	if(!load_gppc_scenario(in))
	{
		WARTHOG_GERROR_FMT(
		    "Failed to load scenario file \"{}\"", sfile_.string());
		throw std::runtime_error("scenario_manager failed to load scenario");
	}
}

void
scenario_manager::load_scenario(
    std::istream& file, std::filesystem::path&& scenfile)
{
	sfile_ = std::move(scenfile);
	if(!load_gppc_scenario(file))
	{
		WARTHOG_GERROR_FMT(
		    "Failed to load scenario file \"{}\"", std::string(sfile_));
		throw std::runtime_error("scenario_manager failed to load scenario");
	}
}

// V1.0 is the version officially supported by HOG
std::expected<void, std::errc>
scenario_manager::load_gppc_scenario(std::istream& scenfile)
{
	clear();

	io::scenario_serialize si;
	si.set_scenario_filename(std::filesystem::path(sfile_));
	si.set_force_int(true);

	// open stream and read header
	if(auto r = si.open_read(&scenfile); !r)
	{
		WARTHOG_GERROR("scenario_manager failed to open for read");
		return std::unexpected(r.error());
	}
	if(auto r = si.read_version(); !r)
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read version on line: {}",
		    si.get_line_num());
		return std::unexpected(r.error());
	}
	version_ = si.get_version();
	if(version_ == io::scenario_version::VERSION_1)
	{
		return load_gppc_scenario_body_v1(si);
	}
	else if(version_ == io::scenario_version::VERSION_2)
	{
		return load_gppc_scenario_body_v2(si);
	}
	else
	{
		WARTHOG_GERROR("scenario_manager reading unsupported version");
		return std::unexpected(std::errc::invalid_argument);
	}
}

std::expected<void, std::errc>
scenario_manager::load_gppc_scenario_body_v1(io::scenario_serialize& si)
{
	if(auto r = si.read_header(); !r)
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read scenario v1 header on line: {}",
		    si.get_line_num());
		return std::unexpected(r.error());
	}

	mfile_ = si.get_map_filename();
	experiments_.reserve(1024);
	commands_.reserve(1024);
	scenario_width_  = si.get_map_width();
	scenario_height_ = si.get_map_height();

	// check cost type
	bool use_cost = true;
	if(!cost_type_.empty())
	{
		if(cost_type_ == "-")
		{
			// use no cost type
			use_cost = false;
		}
		else
		{
			// user-provided cost, error for v1
			WARTHOG_GERROR_FMT(
			    "scenario_manager v1 does not support cost type, was provided "
			    "{}",
			    cost_type_);
			return std::unexpected(std::errc::invalid_argument);
		}
	}

	// setup commands header for static scenario
	commands_.push_back(scenario_command::make_snapshot(0, 0));
	commands_.push_back(scenario_command::make_patch(0, 0, 0, 0));
	patch_count_          += 1;
	snapshot_count_       += 1;
	static_scenario_start_ = (int32_t)commands_.size();
	// read queries until done
	bool first = true;
	io::scenario_instance Q;
	std::string_view map_string;
	while(true)
	{
		Q.reset();
		auto con = si.read_instance_line(Q);
		if(!con)
		{
			WARTHOG_GERROR_FMT(
			    "scenario_manager failed to read inst on line: {}",
			    si.get_line_num());
			return std::unexpected(std::errc::io_error);
		}
		if(*con == io::scenario_serialize::VALID)
		{
			std::string_view current_map(Q.map);
			if(current_map.size() > 2048) // limit string size
			{
				WARTHOG_GERROR_FMT(
				    "scenario_manager v1 inst line map exceeds 2048 chars on "
				    "line: {}",
				    si.get_line_num());
				return std::unexpected(std::errc::argument_out_of_domain);
			}
			if(map_string != current_map)
			{
				map_string = copy_string(current_map);
			}
			std::optional<double> cost_value;
			if(use_cost)
			{
				// add cost
				cost_value.emplace(Q.cost[0]);
			}
			experiment* ex = std::construct_at(
			    static_cast<experiment*>(experiments_res_.allocate(
			        sizeof(experiment), alignof(experiment))),
			    (uint32_t)Q.start_x, (uint32_t)Q.start_y, (uint32_t)Q.goal_x,
			    (uint32_t)Q.goal_y, si.get_map_width(), si.get_map_height(),
			    cost_value, map_string);
			experiments_.push_back(ex);
			commands_.push_back(
			    scenario_command::make_inst(
			        Q.bucket, inst_count_++,
			        (uint32_t)(experiments_.size() - 1)));
		}
		else if(*con == io::scenario_serialize::FINAL) { break; }
		else
		{
			WARTHOG_GWARN_FMT(
			    "scenario_manager v1 invalid instance on line {}",
			    si.get_line_num());
		}
	}
	return {};
}

std::expected<void, std::errc>
scenario_manager::load_gppc_scenario_body_v2(io::scenario_serialize& si)
{
	if(!si.read_header())
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read scenario v2 header on line: {}",
		    si.get_line_num());
		return std::unexpected(std::errc::io_error);
	}

	mfile_ = si.get_map_filename();
	experiments_.reserve(1024);
	commands_.reserve(1024);
	scenario_width_        = si.get_map_width();
	scenario_height_       = si.get_map_height();
	static_scenario_start_ = -1; // init dynamic scenario

	// set map filename
	std::string_view map_string = copy_string(si.get_map_filename().string());
	if(map_string.size() > 2048) // limit string size
	{
		WARTHOG_GERROR("scenario_manager v2 map exceeds 2048 chars");
		return std::unexpected(std::errc::filename_too_long);
	}

	// get cost index
	int cost_index = si.get_cost_type().size() != 0 ? 0 : -1;
	if(!cost_type_.empty())
	{
		if(cost_type_ == "-")
		{
			// use no cost type
			cost_index = -1;
		}
		else
		{
			// user-provided cost index
			cost_index = si.find_cost_index(cost_type_);
			if(cost_index < 0)
			{
				WARTHOG_GERROR_FMT(
				    "scenario_manager v2 failed to find user-provided cost: "
				    "{}",
				    cost_type_);
				return std::unexpected(std::errc::invalid_argument);
			}
		}
	}

	// read queries until done
	io::scenario_instance Q;
	io::scenario_patch P;
	int last_type   = -1;
	int last_bucket = -1;
	while(true)
	{
		// try reading a inst line
		auto con = si.read_instance_line(Q);
		if(!con)
		{
			WARTHOG_GERROR_FMT(
			    "scenario_manager failed to read command on line: {}",
			    si.get_line_num());
			return std::unexpected(std::errc::io_error);
		}
		if(*con == io::scenario_serialize::VALID)
		{
			if(last_type == -1)
			{
				// only used if first command is a inst
				commands_.push_back(
				    scenario_command::make_snapshot(
				        Q.bucket, snapshot_count_++));
			}
			if(last_type == -1
			   || last_type == io::scenario_serialize::CMD_PATCH)
			{
				// set as static scenario (for now)
				static_scenario_start_
				    = static_cast<int32_t>(commands_.size());
			}
			last_type   = io::scenario_serialize::CMD_INST;
			last_bucket = Q.bucket;

			std::optional<double> ex_cost;
			if(cost_index >= 0) { ex_cost = Q.cost[cost_index]; }
			experiment* ex = std::construct_at(
			    static_cast<experiment*>(experiments_res_.allocate(
			        sizeof(experiment), alignof(experiment))),
			    Q.start_x, Q.start_y, Q.goal_x, Q.goal_y, si.get_map_width(),
			    si.get_map_height(), ex_cost, map_string);
			experiments_.push_back(ex);
			commands_.push_back(
			    scenario_command::make_inst(
			        Q.bucket, inst_count_++,
			        (uint32_t)(experiments_.size() - 1)));
		}
		else if(*con == io::scenario_serialize::CMD_PATCH)
		{
			auto pcon = si.read_patch_line(P);
			if(!pcon || *pcon != io::scenario_serialize::VALID)
			{
				WARTHOG_GERROR_FMT(
				    "scenario_manager failed to read command on line: {}",
				    si.get_line_num());
				return std::unexpected(std::errc::io_error);
			}
			if(last_type != io::scenario_serialize::CMD_PATCH
			   || last_bucket != P.bucket)
			{
				commands_.push_back(
				    scenario_command::make_snapshot(
				        P.bucket, snapshot_count_++));
			}
			last_type   = io::scenario_serialize::CMD_PATCH;
			last_bucket = P.bucket;

			commands_.push_back(
			    scenario_command::make_patch(
			        P.bucket, P.patch_id, P.loc_x, P.loc_y));
		}
		else if(*con == io::scenario_serialize::FINAL) { break; }
		else
		{
			WARTHOG_GWARN_FMT_IF(
			    *con == io::scenario_serialize::INVALID,
			    "scenario_manager invalid command on line {}",
			    si.get_line_num());
			WARTHOG_GINFO_FMT_IF(
			    *con != io::scenario_serialize::INVALID,
			    "scenario_manager unknown command on line {}",
			    si.get_line_num());
			// ignore
			if(auto r = si.skip_commands(); !r)
				return std::unexpected(r.error());
		}
	}

	// check if still static scenario
	if(snapshot_count_ != 1)
	{
		// set to dynamic scenario
		static_scenario_start_ = -1;
	}

	return {};
}

std::string_view
scenario_manager::copy_string(std::string_view str)
{
	if(str.empty()) return std::string_view();
	char* mapchars
	    = static_cast<char*>(experiments_res_.allocate(str.size() + 1));
	std::memcpy(mapchars, str.data(), str.size());
	mapchars[str.size()] = '\0';
	return std::string_view(mapchars, str.size());
}

/**
 * Finds a matching map file to a scenario.
 * Take mappath as scenmgr map name.  scendir as partent(sfilename), or
 * current_working_dir. Returns path in order below: If mappath is absolute
 * path: if exists return mappath, else return no path. If scendir/mappath
 * exists: return scendir/mappath. If sfilename != '' and replace sfilename ext
 * to '.map': if exists return that. If sfilename != '' and remove sfilename
 * ext: if new extension is '.map' and exists return that. Return empty path.
 */
std::filesystem::path
find_map_filename(
    const scenario_manager& scenmgr, const std::filesystem::path& sfilename)
{
	namespace fs        = std::filesystem;
	const auto& mapname = scenmgr.get_experiment(0)->map();
	// scen file has a map name designated.
	if(!mapname.empty())
	{
		auto mappath = fs::path(mapname);
		// absolute path, try to use that only.
		if(mappath.is_absolute())
		{
			if(fs::is_regular_file(mappath)) { return mappath; }
			else { return {}; }
		}
		// path is relative path
		auto spath = !sfilename.empty() ? sfilename.parent_path()
		                                : fs::current_path();
		// check relative path from either sfilename directory or
		// current_working_directory
		if(auto append_path = spath / mapname;
		   fs::is_regular_file(append_path))
		{
			return append_path;
		}
	}
	// if a scenario filename was presented, try to deduce map from scenario
	// filename
	if(!sfilename.empty())
	{
		// replace extenion with .map
		auto mapfile = sfilename;
		mapfile.replace_extension(".map");
		if(fs::is_regular_file(mapfile)) return mapfile;
		// remove extension and check it is now .map (test for .map.scen)
		mapfile.replace_extension("");
		if(mapfile.extension() == ".map" && fs::is_regular_file(mapfile))
			return mapfile;
	}
	// no clear way to deduce map, return empty path for no success
	return {};
}
/**
 * Finds a matching map file to a scenario.
 * Take mappath as scenmgr map name.  scendir as partent(sfilename), or
 * current_working_dir. Returns path in order below: If mappath is absolute
 * path: if exists return mappath, else return no path. If scendir/mappath
 * exists: return scendir/mappath. If sfilename != '' and replace sfilename ext
 * to '.map': if exists return that. If sfilename != '' and remove sfilename
 * ext: if new extension is '.map' and exists return that. Return empty path.
 */
std::filesystem::path
find_map_filename(
    const std::filesystem::path& mapname,
    const std::filesystem::path& sfilename)
{
	namespace fs = std::filesystem;
	// scen file has a map name designated.
	if(!mapname.empty())
	{
		auto mappath = fs::path(mapname);
		// absolute path, try to use that only.
		if(mappath.is_absolute())
		{
			if(fs::is_regular_file(mappath)) { return mappath; }
			else { return {}; }
		}
		// path is relative path
		auto spath = !sfilename.empty() ? sfilename.parent_path()
		                                : fs::current_path();
		// check relative path from either sfilename directory or
		// current_working_directory
		if(auto append_path = spath / mapname;
		   fs::is_regular_file(append_path))
		{
			return append_path;
		}
	}
	// if a scenario filename was presented, try to deduce map from scenario
	// filename
	if(!sfilename.empty())
	{
		// replace extenion with .map
		auto mapfile = sfilename;
		mapfile.replace_extension(".map");
		if(fs::is_regular_file(mapfile)) return mapfile;
		// remove extension and check it is now .map (test for .map.scen)
		mapfile.replace_extension("");
		if(mapfile.extension() == ".map" && fs::is_regular_file(mapfile))
			return mapfile;
	}
	// no clear way to deduce map, return empty path for no success
	return {};
}

} // namespace warthog::scenario
