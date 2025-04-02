cmake_minimum_required(VERSION 3.13)

configure_file(cmake/config.h.in include/warthog/config.h @ONLY)
target_sources(warthog_core PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include/warthog/config.h)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/warthog/config.h
	PUBLIC_HEADER
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/warthog)

# use `find include/warthog -maxdepth 1 -type f -name '*.h' | sort`
# use `find include/warthog/*/ -type f -name '*.h' | sort`
target_sources(warthog_core PUBLIC
include/warthog/constants.h
include/warthog/forward.h
include/warthog/limits.h

include/warthog/domain/grid.h
include/warthog/domain/gridmap.h
include/warthog/domain/labelled_gridmap.h

include/warthog/geometry/geography.h
include/warthog/geometry/geom.h

include/warthog/heuristic/heuristic_value.h
include/warthog/heuristic/manhattan_heuristic.h
include/warthog/heuristic/octile_heuristic.h
include/warthog/heuristic/zero_heuristic.h

include/warthog/io/grid.h

include/warthog/memory/arraylist.h
include/warthog/memory/bittable.h
include/warthog/memory/cpool.h
include/warthog/memory/node_pool.h

include/warthog/search/dummy_filter.h
include/warthog/search/dummy_listener.h
include/warthog/search/expansion_policy.h
include/warthog/search/gridmap_expansion_policy.h
include/warthog/search/noop_search.h
include/warthog/search/problem_instance.h
include/warthog/search/search.h
include/warthog/search/search_metrics.h
include/warthog/search/search_node.h
include/warthog/search/search_parameters.h
include/warthog/search/solution.h
include/warthog/search/uds_traits.h
include/warthog/search/unidirectional_search.h
include/warthog/search/vl_gridmap_expansion_policy.h

include/warthog/util/cast.h
include/warthog/util/cost_table.h
include/warthog/util/dimacs_parser.h
include/warthog/util/experiment.h
include/warthog/util/file_utils.h
include/warthog/util/gm_parser.h
include/warthog/util/helpers.h
include/warthog/util/log.h
include/warthog/util/macros.h
include/warthog/util/pqueue.h
include/warthog/util/scenario_manager.h
include/warthog/util/timer.h
include/warthog/util/vec_io.h
)
