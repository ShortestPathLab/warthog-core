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
#ifdef WARTHOG_POSTHOC
#include <warthog/io/grid_trace.h>
#endif

#include "cfg.h"
#include <getopt.h>
#include <warthog/config.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
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
// run only this query, or -1 for all
int filter_id = -1;
#ifdef WARTHOG_POSTHOC
// write trace to file, empty string to disable
std::string trace_file;
using listener_grid = ::warthog::io::grid_trace;
using listener_type = std::tuple<listener_grid>;
#else
using listener_type = std::tuple<>;
#endif

void
help(std::ostream& out)
{
	out << "warthog version " << WARTHOG_VERSION << "\n";
	out << "==> manual <==\n"
	    << "This program solves/generates grid-based pathfinding "
	       "problems using the\n"
	    << "map/scenario format from the 2014 Grid-based Path Planning "
	       "Competition\n\n";

	out << "The following are valid parameters for SOLVING instances:\n"
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
	    << "\t--filter [id] (optional; run only query [id])\n"
#ifdef WARTHOG_POSTHOC
	    << "\t--trace [.trace.yaml file] (optional; write posthoc trace for "
	       "first query to [file])\n"
#endif
	    << "Invoking the program this way solves all instances in [scen "
	       "file] with algorithm [alg]\n"
	    << "Currently recognised values for [alg]:\n"
	    << "\tastar, astar_wgm, astar4c, dijkstra\n";
}

bool
check_optimality(
    warthog::search::solution& sol, warthog::util::experiment* exp)
{
	uint32_t precision = 2;
	double epsilon     = (1.0 / (int)pow(10, precision)) / 2;
	double delta       = fabs(sol.sum_of_edge_costs_ - exp->distance());

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

#ifdef WARTHOG_POSTHOC
#define WARTHOG_POSTHOC_DO(f) f
#else
#define WARTHOG_POSTHOC_DO(f)
#endif

template<typename Search>
int
run_experiments(
    Search& algo, std::string alg_name,
    warthog::util::scenario_manager& scenmgr, bool verbose, bool checkopt,
    std::ostream& out)
{
	WARTHOG_GINFO_FMT("start search with algorithm {}", alg_name);
	warthog::search::search_parameters par;
	warthog::search::solution sol;
	auto* expander = algo.get_expander();
	if(expander == nullptr) return 1;

	out << "id\talg\texpanded\tgenerated\treopen\tsurplus\theapops"
	    << "\tnanos\tplen\tpcost\tscost\tmap\n";
	for(uint32_t i  = filter_id >= 0 ? static_cast<uint32_t>(filter_id) : 0,
	             ie = filter_id >= 0
	        ? i + 1
	        : static_cast<uint32_t>(scenmgr.num_experiments());
	    i < ie; i++)
	{
#ifdef WARTHOG_POSTHOC
		std::optional<std::ofstream>
		    trace_stream; // open and pass to trace if used
		if constexpr(std::same_as<
		                 listener_type,
		                 std::remove_cvref_t<decltype(algo.get_listeners())>>)
		{
			if(i == filter_id && !trace_file.empty())
			{
				listener_grid& l
				    = std::get<listener_grid>(algo.get_listeners());
				trace_stream.emplace(trace_file);
				l.open(*trace_stream);
			}
		}
#endif
		warthog::util::experiment* exp = scenmgr.get_experiment(i);

		warthog::pack_id startid
		    = expander->get_pack(exp->startx(), exp->starty());
		warthog::pack_id goalid
		    = expander->get_pack(exp->goalx(), exp->goaly());
		warthog::search::problem_instance pi(startid, goalid, verbose);
		sol.reset();

		algo.get_path(&pi, &par, &sol);

#ifdef WARTHOG_POSTHOC
		if constexpr(std::same_as<
		                 listener_type,
		                 std::remove_cvref_t<decltype(algo.get_listeners())>>)
		{
			if(trace_stream.has_value())
			{
				// close
				std::get<listener_grid>(algo.get_listeners()).close();
			}
		}
#endif

		out << i << "\t" << alg_name << "\t" << sol.met_.nodes_expanded_
		    << "\t" << sol.met_.nodes_generated_ << "\t"
		    << sol.met_.nodes_reopen_ << "\t" << sol.met_.nodes_surplus_
		    << "\t" << sol.met_.heap_ops_ << "\t"
		    << sol.met_.time_elapsed_nano_.count() << "\t"
		    << (!sol.path_.empty() ? sol.path_.size() - 1 : 0) << "\t"
		    << sol.sum_of_edge_costs_ << "\t" << exp->distance() << "\t"
		    << scenmgr.last_file_loaded() << std::endl;

		if(checkopt)
		{
			if(!check_optimality(sol, exp))
			{
				WARTHOG_GCRIT("search error: failed suboptimal 4");
				return 4;
			}
		}
	}

	WARTHOG_GINFO_FMT(
	    "search complete; total memory: {}", algo.mem() + scenmgr.mem());
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

	warthog::search::unidirectional_search astar(
	    &heuristic, &expander, &open, listener_type(WARTHOG_POSTHOC_DO(&map)));

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	return ret;
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

	warthog::search::unidirectional_search astar(
	    &heuristic, &expander, &open, listener_type(WARTHOG_POSTHOC_DO(&map)));

	int ret = run_experiments(
	    astar, alg_name, scenmgr, verbose, checkopt, std::cout);
	return ret;
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

	struct dijkstra_traits
	{
		using observer = listener_type;
		static consteval auto
		ac()
		{
			return warthog::search::admissibility_criteria::optimal;
		}
	};
	warthog::search::unidirectional_search<
	    decltype(heuristic), decltype(expander), decltype(open),
	    dijkstra_traits>
	    dijkstra(
	        &heuristic, &expander, &open,
	        listener_type(WARTHOG_POSTHOC_DO(&map)));

	int ret = run_experiments(
	    dijkstra, alg_name, scenmgr, verbose, checkopt, std::cout);
	return ret;
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
	return ret;
}

} // namespace

int
main(int argc, char** argv)
{
	// parse arguments
	warthog::util::param valid_args[]
	    = {{"alg", required_argument, 0, 0},
	       {"scen", required_argument, 0, 0},
	       {"map", required_argument, 0, 0},
	       // {"gen", required_argument, 0, 3},
	       {"help", no_argument, &print_help, 1},
	       {"checkopt", no_argument, &checkopt, 1},
	       {"verbose", no_argument, &verbose, 1},
	       {"filter", required_argument, &filter_id, 1},
#ifdef WARTHOG_POSTHOC
	       {"trace", required_argument, 0, 0},
#endif
	       {"costs", required_argument, 0, 1},
	       {0, 0, 0, 0}};

	warthog::util::cfg cfg;
	cfg.parse_args(argc, argv, "a:b:c:def", valid_args);

	if(argc == 1 || print_help)
	{
		help(std::cout);
		return 0;
	}

	std::string sfile = cfg.get_param_value("scen");
	std::string alg   = cfg.get_param_value("alg");
	// std::string gen = cfg.get_param_value("gen");
	std::string mapfile  = cfg.get_param_value("map");
	std::string costfile = cfg.get_param_value("costs");

	if(filter_id == 1)
	{
		filter_id = std::stoi(cfg.get_param_value("filter"));
	}
#ifdef WARTHOG_POSTHOC
	trace_file = cfg.get_param_value("trace");
#endif

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
		help(std::cout);
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
