// warthog.cpp
//
// Pulls together a variety of different algorithms
// for pathfinding on grid graphs.
//
// @author: dharabor
// @created: 2016-11-23
//

#include <warthog/config.h>

#include <warthog/forward.h>

#include <warthog/io/dimacs_parser.h>
#include <warthog/domain/static_adj_graph.h>
#include <warthog/util/string.h>

#ifdef WARTHOG_POSTHOC
#include <warthog/io/grid_trace.h>
#endif

#include "cfg.h"

#include <iostream>
#include <string>

namespace
{
// check computed solutions are optimal
int checkopt = 0;
// print debugging info during search
int verbose = 0;
// display program help on startup
int print_help = 0;
// run only this snapshot, or -1 for all
int snapshot_id = -1;
// run only this inst, or -1 for all
int filter_id = -1;
std::string dump_map;
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
	out << "warthog version " WARTHOG_VERSION "\n";
	out << "==> manual <==\n"
	    << "This program solves/generates grid-based pathfinding "
	       "problems using the\n"
	    << "map/scenario format from the 2014 Grid-based Path Planning "
	       "Competition\n\n";

	out << "The following are valid parameters for SOLVING instances:\n"
	    << "\t--alg [alg] (required)\n"
	    << "\t--scen [p2p file] (required) \n"
	    << "\t--graph [gr file] (required) \n"
	    << "\t--coord [co file] (optional) \n"
	    << "\t--checkopt (optional; compare solution costs against "
	       "values in the scen file)\n"
	    << "\t--verbose (optional; prints debugging info when compiled "
	       "with debug symbols)\n"
	    << "\t--filter [num] (default -1; run all instances (-1) or run "
	       "instance num;\n"
#ifdef WARTHOG_POSTHOC
	    << "\t--trace [.trace.yaml file] (optional; write posthoc trace for "
	       "first instance to [file])\n"
#endif
	    << "Invoking the program this way solves all instances in [p2p file]"
			" with algorithm [alg]\n"
	    << "Currently recognised values for [alg]:\n"
	    << "\tastar, dijkstra\n";
}

#ifdef WARTHOG_POSTHOC
#define WARTHOG_POSTHOC_DO(f) f
#else
#define WARTHOG_POSTHOC_DO(f)
#endif

/// @brief general wrapper around dimics
struct dimics_scenario
{
	warthog::io::dimacs_parser parser;
	warthog::domain::static_adj_graph32<warthog::graph::e_OUT, uint32_t> graph;

	void setup_graph()
	{
		dimics_to_graph(parser, graph);
	}
};

} // namespace

int
main(int argc, char** argv)
{
	// parse arguments
	warthog::util::param valid_args[]
	    = {{"alg", required_argument, 0, 0},
	       {"scen", required_argument, 0, 0},
	       {"graph", required_argument, 0, 0},
	       {"coord", required_argument, 0, 0},
	       // {"gen", required_argument, 0, 3},
	       {"help", no_argument, &print_help, 1},
	       {"checkopt", no_argument, &checkopt, 1},
	       {"verbose", no_argument, &verbose, 1},
	       {"filter", required_argument, &filter_id, 1},
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
	std::string graphfile     = cfg.get_param_value("graph");
	std::string coordfile     = cfg.get_param_value("coord");

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
	if(alg == "" || sfile == "" || graphfile == "")
	{
		help(std::cout);
		return 0;
	}

	// load up the instances
	dimics_scenario dimics;

	if (!dimics.parser.load_graph(graphfile.c_str()))
	{
		WARTHOG_GCRIT("graph file failed to load dimics");
		return (int)std::errc::io_error;
	}
	if (coordfile != "") {
		if (!dimics.parser.load_graph(coordfile.c_str()))
		{
			WARTHOG_GCRIT("coord file failed to load dimics");
			return (int)std::errc::io_error;
		}
	}

	if (!dimics.parser.load_instance(sfile.c_str()))
	{
		WARTHOG_GCRIT("scenario file failed to load dimics");
		return (int)std::errc::io_error;
	}

	dimics.setup_graph();

	if(alg == "astar") {  }
	WARTHOG_GCRIT_FMT("invalid search algorithm: ", alg);
	return (int)std::errc::invalid_argument;
}
