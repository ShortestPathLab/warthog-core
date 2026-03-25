#include <warthog/util/scenario_manager.h>

#include <warthog/search/dummy_listener.h>
#include <warthog/search/problem_instance.h>
#include <warthog/io/log.h>

#include <cstdlib>
#include <cstring>

namespace warthog::util
{

scenario_manager::scenario_manager() { }

scenario_manager::~scenario_manager() = default;

void
scenario_manager::load_scenario(const std::filesystem::path& filelocation)
{
	sfile_ = filelocation;
	std::ifstream in(filelocation);
	if (load_gppc_scenario(in) != std::errc{})
	{
		WARTHOG_GERROR_FMT("Failed to load scenario file \"{}\"", std::string(sfile_));
		throw std::runtime_error("scenario_manager failed to load scenario");
	}
}

void
scenario_manager::load_scenario(std::istream& file, std::filesystem::path&& scenfile)
{
	sfile_ = std::move(scenfile);
	load_gppc_scenario(file);
	if (load_gppc_scenario(file) != std::errc{})
	{
		WARTHOG_GERROR_FMT("Failed to load scenario file \"{}\"", std::string(sfile_));
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
	if (auto ec = si.open_read(&scenfile); ec != std::errc{})
	{
		WARTHOG_GERROR("scenario_manager failed to open for read");
		return ec;
	}
	if (auto ec = si.read_version(); ec != std::errc{}){
		WARTHOG_GERROR_FMT("scenario_manager failed to read version on line: {}", si.get_line_num());
		return std::errc::io_error;
	}
	if (si.get_version() != io::scenario_version::version1)
	{
		WARTHOG_GERROR("scenario_manager reading unsupported version");
		return std::errc::invalid_argument;
	}
	if (auto ec = si.read_header_v1(); ec != std::errc{})
	{
		WARTHOG_GERROR_FMT("scenario_manager failed to read scenario header on line: {}", si.get_line_num());
		return std::errc::io_error;
	}
	
	mfile_ = si.get_map_filename();
	experiments_.clear();
	experiments_.reserve(1024);
	uint32_t width = si.get_map_width();
	uint32_t height = si.get_map_height();
	// read queries until done
	bool first = true;
	io::scenario_query Q; // keep out of loop to reuse std::string
	while (true)
	{
		Q.reset();
		auto [con,ec] = si.read_query_line_v1(Q);
		if (ec != std::errc{})
		{
			WARTHOG_GERROR_FMT("scenario_manager failed to read query on line: {}", si.get_line_num());
			return std::errc::io_error;
		}
		if (con == io::scenario_serialize::valid)
		{
			// no need to check for 
			experiment* ex = std::construct_at(
				static_cast<experiment*>(experiments_res_.allocate(sizeof(experiment), alignof(experiment))),
				(uint32_t)Q.start_x, (uint32_t)Q.start_y,
				(uint32_t)Q.goal_x, (uint32_t)Q.goal_y,
				si.get_map_width(), si.get_map_height(),
				Q.dist[(int)io::dist_type::octile_ncc],
				(first ? std::string(Q.map) : std::string())
			);
			experiments_.push_back(ex);
		} else if (con == io::scenario_serialize::final)
		{
			break;
		}
		first = false; // store map in first query for backward compatability
	}
	return std::errc{};
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

void
scenario_manager::sort()
{
	for(unsigned int i = 0; i < experiments_.size(); i++)
	{
		for(unsigned int j = i; j < experiments_.size(); j++)
		{
			if(experiments_.at(j)->distance() < experiments_.at(i)->distance())
			{
				experiment* tmp    = experiments_.at(i);
				experiments_.at(i) = experiments_.at(j);
				experiments_.at(j) = tmp;
			}
		}
	}
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
    const std::filesystem::path& mapname, const std::filesystem::path& sfilename)
{
	namespace fs        = std::filesystem;
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
