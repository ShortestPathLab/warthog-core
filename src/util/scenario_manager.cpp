#include <warthog/util/scenario_manager.h>

#include <warthog/io/log.h>
#include <warthog/search/dummy_listener.h>
#include <warthog/search/problem_instance.h>

#include <cstdlib>
#include <cstring>

namespace warthog::util
{

scenario_manager::scenario_manager() { }

scenario_manager::~scenario_manager() = default;

void scenario_manager::clear()
{
	experiments_.clear();
	experiments_res_.release();
	sfile_.clear();
	mfile_.clear();
	restart();
}

std::pair<experiment*, int> scenario_manager::experiment_next(uint32_t count)
{
	patches_.clear(); // reset patches
	if (count == 0)
		return {nullptr, 0};
	uint32_t command_size = static_cast<uint32_t>(commands_.size());
	int patch_count = 0;
	while (command_at_ < command_size) {
		auto cmd = commands_[command_at_];
		// command_at_ incremented in following fuction calls
		switch (cmd.type) {
		case scenario_command::SNAPSHOT:
		case scenario_command::PATCH:
			patch_count += snapshot_patches();
			break;
		case scenario_command::QUERY:
			if (experiment* query = snapshot_query(); query != nullptr) {
				if (--count == 0)
					return {query, patch_count};
			}
			break;
		default:
			// should never be reached
			++command_at_;
			WARTHOG_GDEBUG("scenario_manager::experiment_next invalid command type in " WARTHOG_FILENAME_LINE);
		}
		// exits loop 
	}
	// no more experiments
	return {nullptr, patch_count};
}

void
scenario_manager::restart()
{
	patches_.clear();
	version_ = io::scenario_version::UNKNOWN;
	static_scenario_start_ = -1;
	command_at_ = 0;
	experiment_at_ = -1;
	snapshot_at_ = -1;
}

int
scenario_manager::snapshot_next(bool clear_patch)
{
	if (clear_patch)
		patches_.clear();
	const uint32_t command_size = static_cast<uint32_t>(commands_.size());
	while (command_at_ < command_size) {
		auto cmd = commands_[command_at_];
		switch (cmd.type) {
		case scenario_command::SNAPSHOT:
			if (cmd.id != snapshot_at_) {
				// at new snapshot, return
				snapshot_at_ = cmd.id;
				return snapshot_at_;
			}
			// current snapshot, goto next snapshot
		[[fallthrough]];
		case scenario_command::QUERY:
			++snapshot_at_;
			break;
		case scenario_command::PATCH:
			snapshot_patches(false);
			// snapshot_patches increments snapshot_at_
			break;
		default:
			WARTHOG_GDEBUG("scenario_manager::snapshot_next invalid command type in " WARTHOG_FILENAME_LINE);
		}
	}
	return false;
}

int
scenario_manager::snapshot_patches(bool clear_patch)
{
	if (clear_patch)
		patches_.clear();
	const uint32_t command_size = static_cast<uint32_t>(commands_.size());
	int count = 0;
	// if start of snapshot, apply that snapshot patches
	if (command_at_ < command_size && commands_[command_at_].type == scenario_command::SNAPSHOT)
		command_at_ += 1;
	// while command_at_ is PATCH, add patch to applied list
	while (command_at_ < command_size) {
		if (auto cmd = commands_[command_at_]; cmd.bucket == scenario_command::PATCH) {
			command_at_ += 1;
			count += 1;
			patches_.push_back(cmd.id);
		}
	}
	// end at first non-PATCH command
	return count;
}

experiment*
scenario_manager::snapshot_query()
{
	if (command_at_ >= commands_.size())
		return nullptr;
	auto cmd = commands_[command_at_];
	if (cmd.type != scenario_command::QUERY)
		return nullptr;
	command_at_ += 1;
	experiment_at_ += 1;
	if (cmd.cmd.query.query_id >= experiments_.size() || cmd.cmd.query.query_id != (uint32_t)experiment_at_) {
		WARTHOG_GERROR_FMT("scenario_manager::snapshot_query invalid query_id {} to experiment, expected {} (max {}) in {}", cmd.cmd.query.query_id, experiment_at_, experiments_.size(), WARTHOG_FILENAME_LINE);
		return nullptr;
	}
	return experiments_[cmd.cmd.query.query_id];
}

void
scenario_manager::load_scenario(const std::filesystem::path& filelocation)
{
	sfile_ = filelocation;
	std::ifstream in(filelocation);
	if(load_gppc_scenario(in) != std::errc{})
	{
		WARTHOG_GERROR_FMT(
		    "Failed to load scenario file \"{}\"", std::string(sfile_));
		throw std::runtime_error("scenario_manager failed to load scenario");
	}
}

void
scenario_manager::load_scenario(
    std::istream& file, std::filesystem::path&& scenfile)
{
	sfile_ = std::move(scenfile);
	load_gppc_scenario(file);
	if(load_gppc_scenario(file) != std::errc{})
	{
		WARTHOG_GERROR_FMT(
		    "Failed to load scenario file \"{}\"", std::string(sfile_));
		throw std::runtime_error("scenario_manager failed to load scenario");
	}
}

// V1.0 is the version officially supported by HOG
std::errc
scenario_manager::load_gppc_scenario(std::istream& scenfile)
{
	clear();

	io::scenario_serialize si;
	si.set_scenario_filename(std::filesystem::path(sfile_));
	si.set_force_int(true);

	// open stream and read header
	if(auto ec = si.open_read(&scenfile); ec != std::errc{})
	{
		WARTHOG_GERROR("scenario_manager failed to open for read");
		return ec;
	}
	if(auto ec = si.read_version(); ec != std::errc{})
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read version on line: {}",
		    si.get_line_num());
		return std::errc::io_error;
	}
	auto version = si.get_version();
	if (version == io::scenario_version::VERSION_1) {
		return load_gppc_scenario_body_v1(si);
	} else if(version == io::scenario_version::VERSION_2)
	{
		return load_gppc_scenario_body_v2(si);
	} else {
		WARTHOG_GERROR("scenario_manager reading unsupported version");
		return std::errc::invalid_argument;
	}
}

std::errc
scenario_manager::load_gppc_scenario_body_v1(io::scenario_serialize& si)
{

	if(auto ec = si.read_header_v1(); ec != std::errc{})
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read scenario v1 header on line: {}",
		    si.get_line_num());
		return std::errc::io_error;
	}

	mfile_ = si.get_map_filename();
	experiments_.reserve(1024);
	uint32_t width  = si.get_map_width();
	uint32_t height = si.get_map_height();
	// read queries until done
	bool first = true;
	io::scenario_query Q; // keep out of loop to reuse std::string
	std::string_view map_string;
	while(true)
	{
		Q.reset();
		auto [con, ec] = si.read_query_line_v1(Q);
		if(ec != std::errc{})
		{
			WARTHOG_GERROR_FMT(
			    "scenario_manager failed to read query on line: {}",
			    si.get_line_num());
			return std::errc::io_error;
		}
		if(con == io::scenario_serialize::valid)
		{
			std::string_view current_map(Q.map);
			if (current_map.size() > 2048) // limit string size
			{
				WARTHOG_GERROR_FMT(
					"scenario_manager v1 query line map exceeds 2048 chars on line: {}",
					si.get_line_num());
				return std::errc::argument_out_of_domain;
			}
			if (map_string != current_map) {
				map_string = copy_string(current_map);
			}
			experiment* ex = std::construct_at(
			    static_cast<experiment*>(experiments_res_.allocate(
			        sizeof(experiment), alignof(experiment))),
			    (uint32_t)Q.start_x, (uint32_t)Q.start_y, (uint32_t)Q.goal_x,
			    (uint32_t)Q.goal_y, si.get_map_width(), si.get_map_height(),
			    Q.dist[(int)io::dist_type::N_8C_NCC],
			   	map_string);
			experiments_.push_back(ex);
		}
		else if(con == io::scenario_serialize::final) { break; }
	}
	return std::errc{};
}

std::errc
scenario_manager::load_gppc_scenario_body_v2(io::scenario_serialize& si)
{

	if(auto ec = si.read_header_v2(); ec != std::errc{})
	{
		WARTHOG_GERROR_FMT(
		    "scenario_manager failed to read scenario v2 header on line: {}",
		    si.get_line_num());
		return std::errc::io_error;
	}

	mfile_ = si.get_map_filename();
	experiments_.reserve(1024);
	uint32_t width  = si.get_map_width();
	uint32_t height = si.get_map_height();
	// read queries until done
	bool first = true;
	io::scenario_query Q; // keep out of loop to reuse std::string
	std::string_view map_string;
	while(true)
	{
		Q.reset();
		auto [con, ec] = si.read_query_line_v1(Q);
		if(ec != std::errc{})
		{
			WARTHOG_GERROR_FMT(
			    "scenario_manager failed to read query on line: {}",
			    si.get_line_num());
			return std::errc::io_error;
		}
		if(con == io::scenario_serialize::valid)
		{
			std::string_view current_map(Q.map);
			if (current_map.size() > 2048) // limit string size
			{
				WARTHOG_GERROR_FMT(
					"scenario_manager v1 query line map exceeds 2048 chars on line: {}",
					si.get_line_num());
				return std::errc::argument_out_of_domain;
			}
			if (map_string != current_map) {
				map_string = copy_string(current_map);
			}
			experiment* ex = std::construct_at(
			    static_cast<experiment*>(experiments_res_.allocate(
			        sizeof(experiment), alignof(experiment))),
			    (uint32_t)Q.start_x, (uint32_t)Q.start_y, (uint32_t)Q.goal_x,
			    (uint32_t)Q.goal_y, si.get_map_width(), si.get_map_height(),
			    Q.dist[(int)io::dist_type::N_8C_NCC],
			   	map_string);
			experiments_.push_back(ex);
		}
		else if(con == io::scenario_serialize::final) { break; }
	}
	return std::errc{};
}

std::string_view scenario_manager::copy_string(std::string_view str)
{
	if (str.empty())
		return std::string_view();
	char* mapchars = static_cast<char*>(experiments_res_.allocate(str.size() + 1));
	std::memcpy(mapchars, str.data(), str.size());
	mapchars[str.size()] = '\0';
	return std::string_view(mapchars, str.size());
}

void
scenario_manager::write_scenario(std::ostream& scenariofile)
{

	std::cerr << "dumping scenario file..\n";
	if(experiments_.size() == 0) // nothing to write
		return;

	// std::ofstream scenariofile;
	scenariofile.precision(16);
	// scenariofile.open(filelocation, std::ios::out);
	scenariofile << "version 1" << std::endl;

	for(unsigned int i = 0; i < experiments_.size(); i++)
	{
		experiment* cur = experiments_.at(i);
		cur->print(scenariofile);
		scenariofile << std::endl;
	}
	// scenariofile.close();
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

} // namespace warthog::util
