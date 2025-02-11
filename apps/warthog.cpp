// warthog.cpp
//
// Pulls together a variety of different algorithms
// for pathfinding on grid graphs.
//
// @author: dharabor
// @created: 2016-11-23
//

#include <warthog/constants.h>
#include <warthog/domain/gridmap.h>
#include <warthog/domain/labelled_gridmap.h>
#include <warthog/heuristic/manhattan_heuristic.h>
#include <warthog/heuristic/octile_heuristic.h>
#include <warthog/heuristic/zero_heuristic.h>
#include <warthog/search/gridmap_expansion_policy.h>
#include <warthog/search/search.h>
#include <warthog/search/unidirectional_search.h>
#include <warthog/search/vl_gridmap_expansion_policy.h>
#include <warthog/util/pqueue.h>
#include <warthog/util/scenario_manager.h>
#include <warthog/util/timer.h>

#include "cfg.h"
#include "config.h"
#include <getopt.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <memory>
#include <sstream>
#include <unordered_map>

// #include "time_constraints.h"

namespace
{
// check computed solutions are optimal
int checkopt = 0;
// print debugging info during search
int verbose = 0;
// display program help on startup
int print_help = 0;

void
help()
{
	std::cerr << "warthog version " << WARTHOG_VERSION << "\n";
	std::cerr << "==> manual <==\n"
	          << "This program solves/generates grid-based pathfinding "
	             "problems using the\n"
	          << "map/scenario format from the 2014 Grid-based Path Planning "
	             "Competition\n\n";

	std::cerr << "The following are valid parameters for SOLVING instances:\n"
	          << "\t--alg [alg] (required)\n"
	          << "\t--scen [scen file] (required) \n"
	          << "\t--map [map file] (optional; specify this to override map "
	             "values in scen file) \n"
	          << "\t--costs [costs file] (required if using a weighted "
	             "terrain algorithm)\n"
	          << "\t--checkopt (optional; compare solution costs against "
	             "values in the scen file)\n"
	          << "\t--verbose (optional; prints debugging info when compiled "
	             "with debug symbols)\n"
	          << "Invoking the program this way solves all instances in [scen "
	             "file] with algorithm [alg]\n"
	          << "Currently recognised values for [alg]:\n"
	          << "\tastar, astar_wgm, astar4c, dijkstra\n";
	// << ""
	// << "The following are valid parameters for GENERATING instances:\n"
	// << "\t --gen [map file (required)]\n"
	// << "Invoking the program this way generates at random 1000 valid
	// problems for \n"
	// << "gridmap [map file]\n";
}

bool
check_optimality(
    warthog::search::solution& sol, warthog::util::experiment* exp)
{
	uint32_t precision = 2;
	double epsilon = (1.0 / (int)pow(10, precision)) / 2;
	double delta = fabs(sol.sum_of_edge_costs_ - exp->distance());

	if(fabs(delta - epsilon) > epsilon)
	{
		std::stringstream strpathlen;
		strpathlen << std::fixed << std::setprecision(exp->precision());
		strpathlen << sol.sum_of_edge_costs_;

		std::stringstream stroptlen;
		stroptlen << std::fixed << std::setprecision(exp->precision());
		stroptlen << exp->distance();

		std::cerr << std::setprecision(exp->precision());
		std::cerr << "optimality check failed!" << std::endl;
		std::cerr << std::endl;
		std::cerr << "optimal path length: " << stroptlen.str()
		          << " computed length: ";
		std::cerr << strpathlen.str() << std::endl;
		std::cerr << "precision: " << precision << " epsilon: " << epsilon
		          << std::endl;
		std::cerr << "delta: " << delta << std::endl;
		return false;
	}
	return true;
}

template<typename Search>
int
run_experiments(
    Search& algo, std::string alg_name,
    warthog::util::scenario_manager& scenmgr, bool verbose, bool checkopt,
    std::ostream& out)
{
	auto* expander = algo.get_expander();
	if(expander == nullptr) return 1;
	out << "id\talg\texpanded\tgenerated\treopen\tsurplus\theapops"
	    << "\tnanos\tplen\tpcost\tscost\tmap\n";
	for(unsigned int i = 0; i < scenmgr.num_experiments(); i++)
	{
		warthog::util::experiment* exp = scenmgr.get_experiment(i);

		warthog::pack_id startid
		    = expander->get_pack(exp->startx(), exp->starty());
		warthog::pack_id goalid
		    = expander->get_pack(exp->goalx(), exp->goaly());
		warthog::search::problem_instance pi(startid, goalid, verbose);
		warthog::search::search_parameters par;
		warthog::search::solution sol;

		algo.get_path(&pi, &par, &sol);

		out << i << "\t" << alg_name << "\t" << sol.met_.nodes_expanded_
		    << "\t" << sol.met_.nodes_generated_ << "\t"
		    << sol.met_.nodes_reopen_ << "\t" << sol.met_.nodes_surplus_
		    << "\t" << sol.met_.heap_ops_ << "\t"
		    << sol.met_.time_elapsed_nano_.count() << "\t"
		    << (sol.path_.size() - 1) << "\t" << sol.sum_of_edge_costs_ << "\t"
		    << exp->distance() << "\t" << scenmgr.last_file_loaded()
		    << std::endl;

		if(checkopt)
		{
			if(!check_optimality(sol, exp)) return 4;
		}
	}

	return 0;
}

int
run_astar(
    warthog::util::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	warthog::domain::gridmap map(mapname.c_str());
	warthog::search::gridmap_expansion_policy expander(&map);
	warthog::heuristic::octile_heuristic heuristic(map.width(), map.height());
	warthog::util::pqueue_min open;

	warthog::search::unidirectional_search astar(&heuristic, &expander, &open);

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	if(ret != 0)
	{
		std::cerr << "run_experiments error code " << ret << std::endl;
		return ret;
	}
	std::cerr << "done. total memory: " << astar.mem() + scenmgr.mem() << "\n";
	return 0;
}

int
run_astar4c(
    warthog::util::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	warthog::domain::gridmap map(mapname.c_str());
	warthog::search::gridmap_expansion_policy expander(&map, true);
	warthog::heuristic::manhattan_heuristic heuristic(
	    map.width(), map.height());
	warthog::util::pqueue_min open;

	warthog::search::unidirectional_search astar(&heuristic, &expander, &open);

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	if(ret != 0)
	{
		std::cerr << "run_experiments error code " << ret << std::endl;
		return ret;
	}
	std::cerr << "done. total memory: " << astar.mem() + scenmgr.mem() << "\n";
	return 0;
}

int
run_dijkstra(
    warthog::util::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	warthog::domain::gridmap map(mapname.c_str());
	warthog::search::gridmap_expansion_policy expander(&map);
	warthog::heuristic::zero_heuristic heuristic;
	warthog::util::pqueue_min open;

	warthog::search::unidirectional_search astar(&heuristic, &expander, &open);

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	if(ret != 0)
	{
		std::cerr << "run_experiments error code " << ret << std::endl;
		return ret;
	}
	std::cerr << "done. total memory: " << astar.mem() + scenmgr.mem() << "\n";
	return 0;
}

int
run_wgm_astar(
    warthog::util::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name, std::string costfile)
{
	warthog::util::cost_table costs(costfile.c_str());
	warthog::domain::vl_gridmap map(mapname.c_str());
	warthog::search::vl_gridmap_expansion_policy expander(&map, costs);
	warthog::heuristic::octile_heuristic heuristic(map.width(), map.height());
	warthog::util::pqueue_min open;

	double lowest_cost = costs.lowest_cost(map);
	if(std::isnan(lowest_cost))
	{
		std::cerr << "err; costs file does not specify cost of some terrains"
		          << std::endl;
		exit(1);
	}
	heuristic.set_hscale(lowest_cost);

	warthog::search::unidirectional_search astar(&heuristic, &expander, &open);

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	if(ret != 0)
	{
		std::cerr << "run_experiments error code " << ret << std::endl;
		return ret;
	}
	std::cerr << "done. total memory: " << astar.mem() + scenmgr.mem() << "\n";
	return 0;
}

} // namespace

int
main(int argc, char** argv)
{
	// parse arguments
	warthog::util::param valid_args[]
	    = {{"alg", required_argument, 0, 1},
	       {"scen", required_argument, 0, 0},
	       {"map", required_argument, 0, 1},
	       // {"gen", required_argument, 0, 3},
	       {"help", no_argument, &print_help, 1},
	       {"checkopt", no_argument, &checkopt, 1},
	       {"verbose", no_argument, &verbose, 1},
	       {"costs", required_argument, 0, 1},
	       {0, 0, 0, 0}};

	warthog::util::cfg cfg;
	cfg.parse_args(argc, argv, "a:b:c:def", valid_args);

	if(argc == 1 || print_help)
	{
		help();
		return 0;
	}

	std::string sfile = cfg.get_param_value("scen");
	std::string alg = cfg.get_param_value("alg");
	// std::string gen = cfg.get_param_value("gen");
	std::string mapfile = cfg.get_param_value("map");
	std::string costfile = cfg.get_param_value("costs");

	// if(gen != "")
	// {
	// 	warthog::util::scenario_manager sm;
	// 	warthog::domain::gridmap gm(gen.c_str());
	// 	sm.generate_experiments(&gm, 1000) ;
	// 	sm.write_scenario(std::cout);
	//     exit(0);
	// }

	// running experiments
	if(alg == "" || sfile == "")
	{
		help();
		return 0;
	}

	// load up the instances
	warthog::util::scenario_manager scenmgr;
	scenmgr.load_scenario(sfile.c_str());

	if(scenmgr.num_experiments() == 0)
	{
		std::cerr << "err; scenario file does not contain any instances\n";
		return 1;
	}

	// the map filename can be given or (default) taken from the scenario file
	if(mapfile == "")
	{
		// first, try to load the map from the scenario file
		mapfile = warthog::util::find_map_filename(scenmgr, sfile);
		if(mapfile.empty())
		{
			std::cerr << "could not locate a corresponding map file\n";
			help(std::cout);
			return 0;
		}
	}
	std::cerr << "mapfile=" << mapfile << std::endl;

	if(alg == "dijkstra") { return run_dijkstra(scenmgr, mapfile, alg); }
	else if(alg == "astar") { return run_astar(scenmgr, mapfile, alg); }
	else if(alg == "astar4c") { return run_astar4c(scenmgr, mapfile, alg); }
	else if(alg == "astar_wgm")
	{
		return run_wgm_astar(scenmgr, mapfile, alg, costfile);
	}
	std::cerr << "err; invalid search algorithm: " << alg << "\n";
	return 1;
}
