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
#include <warthog/scenario/grid_patch_set.h>
#include <warthog/scenario/scenario_manager.h>
#include <warthog/scenario/scenario_runner.h>
#include <warthog/search/gridmap_expansion_policy.h>
#include <warthog/search/search.h>
#include <warthog/search/unidirectional_search.h>
#include <warthog/search/vl_gridmap_expansion_policy.h>
#include <warthog/util/pqueue.h>
#include <warthog/util/string.h>
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
// run only this inst, or -1 for all
int filter_id = -1;
// dump map at id if set
int dump_map_id = -1;
std::string dump_map_file;
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
	    << "\t--grid-weight [file] (required if using a weighted "
	       "terrain algorithm)\n"
	    << "\t--cost [type] (optional; force use of selected solution cost of "
	       "instance, error if not exists)"
	    << "\t--checkopt (optional; compare solution costs against "
	       "values in the scen file)\n"
	    << "\t--verbose (optional; prints debugging info when compiled "
	       "with debug symbols)\n"
	    << "\t--filter [id] (optional; run only inst [id])\n"
	    << "\t--dump-map [id] (optional; dump current gridmap at start of "
	       "inst id to stderr default)\n"
	    << "\t--dump-map-file [filename] (optional; override dump map to "
	       "file)\n"
#ifdef WARTHOG_POSTHOC
	    << "\t--trace [.trace.yaml file] (optional; write posthoc trace for "
	       "first instance to [file])\n"
#endif
	    << "Invoking the program this way solves all instances in [scen "
	       "file] with algorithm [alg]\n"
	    << "Currently recognised values for [alg]:\n"
	    << "\tastar, astar_wgm, astar4c, dijkstra\n"
	    << "Currently recognised values for [v2-cost]:\n"
	    << "8c-ncc (default), 8c-cc, 4c, aa-ncc, aa-cc\n"
	    << "8c = 8-connected, 4c = 4-connected, aa = anyangle, ncc = "
	       "no-corner-cut, cc = corner-cut\n";
}

bool
check_optimality(
    const warthog::search::solution& sol,
    const warthog::scenario::experiment* exp)
{
	if(!exp->distance())
	{
		// unknown solution
		return true;
	}
	constexpr int32_t precision = 2;
	double epsilon              = std::pow(10.0, -precision) * 0.5;
	double delta = std::fabs(sol.sum_of_edge_costs_ - *exp->distance());

	if(delta > epsilon)
	{
		std::cerr << "optimality check failed!" << std::endl;
		std::cerr << std::endl;
		std::cerr << "optimal path length: " << sol.sum_of_edge_costs_
		          << " computed length: ";
		std::cerr << *exp->distance() << std::endl;
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

// convenience wrapper around initialisation code
struct gridmap_scenario
{
	bool dynamic = false;
	const warthog::scenario::scenario_manager* mgr;
	warthog::scenario::scenario_runner run;
	warthog::domain::gridmap grid;
	warthog::scenario::grid_patch_set patches;

	gridmap_scenario(const warthog::scenario::scenario_manager& scen)
	    : mgr(&scen), run(&scen)
	{ }

	bool
	load_map(const std::filesystem::path map)
	{
		dynamic = true;
		if(!patches.load(map)) { return false; }
		return run.gridmap_init(grid, patches);
	}

	bool
	apply_patches()
	{
		if(!dynamic) return true;
		return run.gridmap_apply_patches(grid, patches) >= 0;
	}
};

template<typename Search>
int
run_experiments(
    Search& algo, std::string alg_name, gridmap_scenario& scen, bool verbose,
    bool checkopt, std::ostream& out)
{
	WARTHOG_GINFO_FMT("start search with algorithm {}", alg_name);
	warthog::search::search_parameters par;
	warthog::search::solution sol;
	auto* expander = algo.get_expander();
	if(expander == nullptr) return (int)std::errc::invalid_argument;

	out << "id\tsnapshot\talg\texpanded\tgenerated\treopen\tsurplus\theapops"
	    << "\tnanos\tplen\tpcost\tscost\tmap\n";

	for(uint32_t i = 0;; ++i)
	{
#ifdef WARTHOG_POSTHOC
		std::optional<std::ofstream>
		    trace_stream; // open and pass to trace if used

#endif
		auto [exp, patch_count] = scen.run.experiment_next();
		if(exp == nullptr) { break; }
		if(patch_count != 0)
		{
			if(!scen.apply_patches())
			{
				// failed to apply patches, exit
				WARTHOG_GCRIT("dynamic patch error: failed to apply patches");
				return (int)std::errc::io_error;
			}
		}

		if(i == dump_map_id)
		{
			// print map
			scen.grid.save(dump_map_file, false);
		}

		if(filter_id >= 0 && i == filter_id)
		{
			// trace
#ifdef WARTHOG_POSTHOC
			if constexpr(std::same_as<
			                 listener_type,
			                 std::remove_cvref_t<
			                     decltype(algo.get_listeners())>>)
			{
				if(!trace_file.empty())
				{
					listener_grid& l
					    = std::get<listener_grid>(algo.get_listeners());
					trace_stream.emplace(trace_file);
					l.open(*trace_stream);
				}
			}
#endif
		}
		else if(filter_id >= 0) { continue; }

		warthog::pack_id startid
		    = expander->get_pack(exp->startx(), exp->starty());
		warthog::pack_id goalid
		    = expander->get_pack(exp->goalx(), exp->goaly());
		warthog::search::problem_instance pi(startid, goalid, verbose);
		sol.reset();

		algo.get_path(&pi, &par, &sol);
		// check for no solution
		if(sol.sum_of_edge_costs_ >= warthog::COST_MAX)
		{
			sol.sum_of_edge_costs_ = -1;
		}

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

		out << i << "\t" << scen.run.get_snapshot_at() << "\t" << alg_name
		    << "\t" << sol.met_.nodes_expanded_ << "\t"
		    << sol.met_.nodes_generated_ << "\t" << sol.met_.nodes_reopen_
		    << "\t" << sol.met_.nodes_surplus_ << "\t" << sol.met_.heap_ops_
		    << "\t" << sol.met_.time_elapsed_nano_.count() << "\t"
		    << (!sol.path_.empty() ? sol.path_.size() - 1 : 0) << "\t"
		    << sol.sum_of_edge_costs_ << "\t";
		if(exp->distance())
			out << *exp->distance();
		else
			out << '-';
		out << "\t" << scen.mgr->last_file_loaded() << std::endl;

		if(checkopt)
		{
			if(!check_optimality(sol, exp))
			{
				WARTHOG_GCRIT("search error: failed suboptimal 4");
				return (int)std::errc::result_out_of_range;
			}
		}
	}

	WARTHOG_GINFO_FMT(
	    "search complete; total memory: {}", algo.mem() + scen.mgr->mem());
	return 0;
}

int
run_astar(
    warthog::scenario::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	gridmap_scenario scen(scenmgr);
	if(!scen.load_map(std::filesystem::path(mapname)))
	{
		WARTHOG_GCRIT("failed to load map");
		return (int)std::errc::io_error;
	}
	warthog::search::gridmap_expansion_policy expander(&scen.grid);
	warthog::heuristic::octile_heuristic heuristic(
	    scen.grid.width(), scen.grid.height());
	warthog::util::pqueue_min open;

	warthog::search::unidirectional_search astar(
	    &heuristic, &expander, &open,
	    listener_type(WARTHOG_POSTHOC_DO(&scen.grid)));

	int ret
	    = run_experiments(astar, alg_name, scen, verbose, checkopt, std::cout);
	return ret;
}

int
run_astar4c(
    warthog::scenario::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	gridmap_scenario scen(scenmgr);
	if(!scen.load_map(std::filesystem::path(mapname)))
	{
		WARTHOG_GCRIT("failed to load map");
		return (int)std::errc::io_error;
	}
	warthog::search::gridmap_expansion_policy expander(&scen.grid, true);
	warthog::heuristic::manhattan_heuristic heuristic(
	    scen.grid.width(), scen.grid.height());
	warthog::util::pqueue_min open;

	warthog::search::unidirectional_search astar(
	    &heuristic, &expander, &open,
	    listener_type(WARTHOG_POSTHOC_DO(&scen.grid)));

	int ret
	    = run_experiments(astar, alg_name, scen, verbose, checkopt, std::cout);
	return ret;
}

int
run_dijkstra(
    warthog::scenario::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name)
{
	gridmap_scenario scen(scenmgr);
	if(!scen.load_map(std::filesystem::path(mapname)))
	{
		WARTHOG_GCRIT("failed to load map");
		return (int)std::errc::io_error;
	}
	warthog::search::gridmap_expansion_policy expander(&scen.grid);
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
	        listener_type(WARTHOG_POSTHOC_DO(&scen.grid)));

	int ret = run_experiments(
	    dijkstra, alg_name, scen, verbose, checkopt, std::cout);
	return ret;
}

int
run_wgm_astar(
    warthog::scenario::scenario_manager& scenmgr, std::string mapname,
    std::string alg_name, std::string costfile)
{
	gridmap_scenario scen(scenmgr);
	// do not load map here
	warthog::util::cost_table costs(costfile.c_str());
	warthog::domain::vl_gridmap map(mapname.c_str());
	warthog::search::vl_gridmap_expansion_policy expander(&map, costs);
	warthog::heuristic::octile_heuristic heuristic(map.width(), map.height());
	warthog::util::pqueue_min open;

	double lowest_cost = costs.lowest_cost(map);
	if(std::isnan(lowest_cost))
	{
		WARTHOG_GCRIT(
		    "grid weights file does not specify cost of some terrains");
		return (int)std::errc::io_error;
	}
	heuristic.set_hscale(lowest_cost);

	warthog::search::unidirectional_search astar(&heuristic, &expander, &open);

	int ret
	    = run_experiments(astar, alg_name, scen, verbose, checkopt, std::cout);
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
	       {"grid-weight", required_argument, 0, 0},
	       // {"gen", required_argument, 0, 3},
	       {"help", no_argument, &print_help, 1},
	       {"checkopt", no_argument, &checkopt, 1},
	       {"cost", required_argument, 0, 0},
	       {"verbose", no_argument, &verbose, 1},
	       {"filter", required_argument, &filter_id, 1},
	       {"dump-map", required_argument, &dump_map_id, 1},
	       {"dump-map-file", required_argument, 0, 0},
#ifdef WARTHOG_POSTHOC
	       {"trace", required_argument, 0, 0},
#endif
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
	std::string mapfile     = cfg.get_param_value("map");
	std::string costtype    = cfg.get_param_value("cost");
	std::string weightsfile = cfg.get_param_value("grid-weight");
	dump_map_file           = cfg.get_param_value("dump-map-file");

	if(filter_id == 1)
	{
		if(warthog::util::parse_token(cfg.get_param_value("filter"), filter_id)
		   != std::errc{})
		{
			WARTHOG_GERROR_FMT("invalid --filter argument {}", filter_id);
			return (int)std::errc::invalid_argument;
		}
	}
#ifdef WARTHOG_POSTHOC
	trace_file = cfg.get_param_value("trace");
#endif

	// running experiments
	if(alg == "" || sfile == "")
	{
		help(std::cout);
		return 0;
	}

	if(dump_map_id == 1)
	{
		if(warthog::util::parse_token(
		       cfg.get_param_value("dump-map"), dump_map_id)
		   != std::errc{})
		{
			WARTHOG_GCRIT_FMT("invalid --dump-map argument {}", dump_map_id);
			return (int)std::errc::invalid_argument;
		}
	}
	if(dump_map_file.empty()) { dump_map_file = "/dev/stderr"; }

	// load up the instances
	warthog::scenario::scenario_manager scenmgr;
	scenmgr.set_cost_type(costtype);
	try
	{
		scenmgr.load_scenario(sfile.c_str());
	}
	catch(const std::runtime_error& e)
	{
		return (int)std::errc::io_error;
	}

	if(scenmgr.num_experiments() == 0)
	{
		WARTHOG_GCRIT("scenario file does not contain any instances");
		return (int)std::errc::invalid_argument;
	}

	// the map filename can be given or (default) taken from the scenario file
	if(mapfile == "")
	{
		// first, try to load the map from the scenario file
		mapfile = warthog::scenario::find_map_filename(scenmgr, sfile);
		if(mapfile.empty())
		{
			std::cerr << "could not locate a corresponding map file\n";
			help(std::cout);
			return 0;
		}
		WARTHOG_GINFO_FMT("deduced mapfile: ", mapfile);
	}

	if(alg == "dijkstra") { return run_dijkstra(scenmgr, mapfile, alg); }
	else if(alg == "astar") { return run_astar(scenmgr, mapfile, alg); }
	else if(alg == "astar4c") { return run_astar4c(scenmgr, mapfile, alg); }
	else if(alg == "astar_wgm")
	{
		return run_wgm_astar(scenmgr, mapfile, alg, weightsfile);
	}
	WARTHOG_GCRIT_FMT("invalid search algorithm: ", alg);
	return (int)std::errc::invalid_argument;
}
