#ifndef WARTHOG_SEARCH_SEARCH_METRICS_H
#define WARTHOG_SEARCH_SEARCH_METRICS_H

// search/search_metrics.h
//
// A container for all the metrics that are recorded when running a search
//
// @author: dharabor
// @created: 2021-10-13
//

#include <iostream>
#include <warthog/constants.h>

namespace warthog::search
{

struct search_metrics
{
	search_metrics() { reset(); }

	search_metrics&
	operator=(const search_metrics& other)
	{
		time_elapsed_nano_ = other.time_elapsed_nano_;
		nodes_expanded_ = other.nodes_expanded_;
		nodes_generated_ = other.nodes_generated_;
		nodes_surplus_ = other.nodes_surplus_;
		nodes_reopen_ = other.nodes_reopen_;
		heap_ops_ = other.heap_ops_;
		lb_ = other.lb_;
		ub_ = other.ub_;
		return *this;
	}

	inline void
	reset()
	{
		time_elapsed_nano_ = 0;
		nodes_expanded_ = 0;
		nodes_generated_ = 0;
		nodes_surplus_ = 0;
		nodes_reopen_ = 0;
		heap_ops_ = 0;
		lb_ = warthog::COST_MAX;
		ub_ = warthog::COST_MAX;
	}

	double time_elapsed_nano_;
	uint32_t nodes_expanded_;
	uint32_t nodes_generated_;
	uint32_t nodes_surplus_;
	uint32_t nodes_reopen_;
	uint32_t heap_ops_;

	// bounds established during the search
	cost_t lb_;
	cost_t ub_;

	// friend std::ostream& operator<<(std::ostream& str,
	// warthog::search_metrics& met);
};

} // namespace warthog::search

std::ostream&
operator<<(std::ostream& str, const warthog::search::search_metrics& met);

#endif // WARTHOG_SEARCH_SEARCH_METRICS_H
