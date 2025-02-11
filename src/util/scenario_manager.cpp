#include <warthog/search/dummy_listener.h>
#include <warthog/search/problem_instance.h>
#include <warthog/util/scenario_manager.h>

#include <cstdlib>
#include <cstring>

namespace warthog::util
{

scenario_manager::scenario_manager() { }

scenario_manager::~scenario_manager()
{
	for(unsigned int i = 0; i < experiments_.size(); i++)
	{
		delete experiments_[i];
	}
	experiments_.clear();
}

void
scenario_manager::load_scenario(const char* filelocation)
{
	std::ifstream infile;
	infile.open(filelocation, std::ios::in);

	if(!infile.good())
	{
		std::cerr << "err; scenario_manager::load_scenario "
		          << "Invalid scenario file: " << filelocation << std::endl;
		infile.close();
		exit(1);
	}

	sfile_ = filelocation;

	char buf[1024];
	infile.getline(buf, 1024);
	if(strstr(buf, "version 1"))
	{
		// GPPC format scenarios
		load_gppc_scenario(infile);
	}
	else
	{
		std::cerr << "err; scenario_manager::load_scenario "
		          << " scenario file not in GPPC format\n";
		infile.close();
		exit(1);
	}
	infile.close();
}

// V1.0 is the version officially supported by HOG
void
scenario_manager::load_gppc_scenario(std::ifstream& infile)
{
	uint32_t sizeX = 0, sizeY = 0;
	uint32_t bucket;
	std::string map;
	uint32_t xs, ys, xg, yg;
	std::string dist;

	while(infile >> bucket >> map >> sizeX >> sizeY >> xs >> ys >> xg >> yg
	      >> dist)
	{
		double dbl_dist = strtod(dist.c_str(), 0);
		experiments_.push_back(
		    new experiment(xs, ys, xg, yg, sizeX, sizeY, dbl_dist, map));

		int32_t precision = 0;
		if(dist.find(".") != std::string::npos)
		{
			precision = ((int32_t)dist.size() - (int32_t)(dist.find(".") + 1));
		}
		experiments_.back()->set_precision(precision);
	}
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
				experiment* tmp = experiments_.at(i);
				experiments_.at(i) = experiments_.at(j);
				experiments_.at(j) = tmp;
			}
		}
	}
}

std::filesystem::path find_map_filename(const scenario_manager& scenmgr, std::filesystem::path sfilename)
{
	namespace fs = std::filesystem;
	const auto& mapname = scenmgr.get_experiment(0)->map();
	bool replace_sfilename = false;
	bool load_mapname = false;
	if (mapname.empty()) {
		replace_sfilename = true;
	} else {
		auto mappath = fs::path(mapname);
		if (mappath.is_absolute()) {
			if (fs::is_regular_file(mappath)) {
				return mappath;
			} else {
				return {};
			}
		}
		// is relative path
		auto spath = !sfilename.empty() ? sfilename.parent_path() : fs::current_path();
		if (auto append_path = spath / mapname; fs::is_regular_file(append_path)) {
			return append_path;
		}
		replace_sfilename = true;
	}
	if (replace_sfilename && !sfilename.empty()) {
		auto mapfile = sfilename.replace_extension(".map");
		if (fs::is_regular_file(mapfile))
			return mapfile;
		mapfile = sfilename.replace_extension("");
		if (fs::is_regular_file(mapfile))
			return mapfile;
	}
	return {};
}

} // namespace warthog::util
