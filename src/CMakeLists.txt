cmake_minimum_required(VERSION 3.13)

target_sources(warthog_core PRIVATE
domain/gridmap.cpp

geometry/geography.cpp
geometry/geom.cpp

io/grid.cpp
io/listeners.cpp

memory/node_pool.cpp

search/expansion_policy.cpp
search/gridmap_expansion_policy.cpp
search/problem_instance.cpp
search/search_metrics.cpp
search/search_node.cpp
search/solution.cpp
search/vl_gridmap_expansion_policy.cpp

util/cost_table.cpp
util/dimacs_parser.cpp
util/experiment.cpp
util/file_utils.cpp
util/gm_parser.cpp
util/helpers.cpp
util/scenario_manager.cpp
util/timer.cpp

)
