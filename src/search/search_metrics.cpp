#include <warthog/search/search_metrics.h>

std::ostream&
operator<<(std::ostream& str, warthog::search::search_metrics& met)
{
	str << " time_elapsed_nano=" << met.time_elapsed_nano_
	    << " nodes expanded=" << met.nodes_expanded_
	    << " touched=" << met.nodes_generated_
	    << " reopened=" << met.nodes_reopen_
	    << " surplus=" << met.nodes_surplus_ << " heap-ops=" << met.heap_ops_
	    << " lb=" << met.lb_ << " ub=" << met.ub_;
	return str;
}
