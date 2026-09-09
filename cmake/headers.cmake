cmake_minimum_required(VERSION 3.13)

# find include/warthog/ -type f | awk -F/ '{print NF"/"$0}' | LANG= sort -nft/ | cut -d/ -f3-
set(WARTHOG_CORE_HEADERS
warthog/constants.h
warthog/defines.h
warthog/forward.h
warthog/limits.h

warthog/domain/grid.h
warthog/domain/gridmap.h
warthog/domain/labelled_gridmap.h

warthog/geometry/geography.h
warthog/geometry/geom.h

warthog/heuristic/heuristic_value.h
warthog/heuristic/manhattan_heuristic.h
warthog/heuristic/octile_heuristic.h
warthog/heuristic/zero_heuristic.h

warthog/io/bittable_serialize.h
warthog/io/fwd.h
warthog/io/grid_trace.h
warthog/io/log.h
warthog/io/observer.h
warthog/io/posthoc_trace.h
warthog/io/scenario_serialize.h
warthog/io/serialize_base.h
warthog/io/stream_observer.h

warthog/memory/arraylist.h
warthog/memory/bittable.h
warthog/memory/cpool.h
warthog/memory/node_pool.h

warthog/scenario/experiment.h
warthog/scenario/grid_patch_set.h
warthog/scenario/scenario_manager.h
warthog/scenario/scenario_runner.h

warthog/search/expansion_policy.h
warthog/search/gridmap_expansion_policy.h
warthog/search/problem_instance.h
warthog/search/search.h
warthog/search/search_metrics.h
warthog/search/search_node.h
warthog/search/search_parameters.h
warthog/search/solution.h
warthog/search/uds_traits.h
warthog/search/unidirectional_search.h
warthog/search/vl_gridmap_expansion_policy.h

warthog/util/cast.h
warthog/util/cost_table.h
warthog/util/dimacs_parser.h
warthog/util/helpers.h
warthog/util/intrin.h
warthog/util/pqueue.h
warthog/util/string.h
warthog/util/template.h
warthog/util/timer.h
)
