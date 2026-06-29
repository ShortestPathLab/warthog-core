cmake_minimum_required(VERSION 3.13)

target_sources(warthog_core PRIVATE
domain/gridmap.cpp

geometry/geography.cpp
geometry/geom.cpp

io/bittable_serialize.cpp
io/grid_trace.cpp
io/log.cpp
io/scenario_serialize.cpp
io/serialize_base.cpp

memory/node_pool.cpp

scenario/grid_patch_set.cpp
scenario/scenario_manager.cpp
scenario/scenario_runner.cpp

search/expansion_policy.cpp
search/gridmap_expansion_policy.cpp
search/problem_instance.cpp
search/search_metrics.cpp
search/search_node.cpp
search/solution.cpp
search/vl_gridmap_expansion_policy.cpp

util/cost_table.cpp
util/dimacs_parser.cpp
util/helpers.cpp
util/timer.cpp
)
