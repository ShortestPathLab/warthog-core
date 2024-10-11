#ifndef WARTHOG_SEARCH_SOLUTION_H
#define WARTHOG_SEARCH_SOLUTION_H

// a wrapper for solutions found by algorithms implemented
// in the warthog library
//
// @author: dharabor
// @created: 2017-05-03
//

#include "search_metrics.h"
#include "search_node.h"

#include <ostream>
#include <vector>
#include <warthog/constants.h>

namespace warthog::search
{

class solution
{
public:
	solution()
	{
		reset();
		path_.reserve(1024);
	}

	solution(const solution& other) : met_(other.met_), path_(other.path_) { }

	inline void
	print_metrics(std::ostream& out) const
	{
		out << "sum_of_edge_costs=" << sum_of_edge_costs_;
		out << met_;
	}

	inline void
	print_path(std::ostream& out) const
	{
		out << "path=";
		for(auto& state : path_)
		{
			out << sn_id_t{state} << " ";
		}
	}

	void
	reset()
	{
		s_node_ = 0;
		sum_of_edge_costs_ = warthog::COST_MAX;
		path_.clear();
		met_.reset();
	}

	// friend std::ostream&
	// ::operator<<(std::ostream& str, solution& sol);

	// search performance metrics
	search::search_metrics met_;

	// the solution itself. we store the incumbent node
	// which produced the solution and the concrete path,
	// from start to target
	search::search_node* s_node_;
	cost_t sum_of_edge_costs_;
	std::vector<pack_id> path_;
};

} // namespace warthog::search

std::ostream&
operator<<(std::ostream& str, const warthog::search::solution& sol);

#endif // WARTHOG_SEARCH_SOLUTION_H
