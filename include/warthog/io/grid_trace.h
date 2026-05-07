#ifndef WARTHOG_IO_GRID_TRACE_H
#define WARTHOG_IO_GRID_TRACE_H

/// @file io/grid_trace.h
///
/// Basic posthoc_trace for use with gridmap.
///
/// @author: Ryan Hechenberger
/// @created: 2025-08-07

#include "posthoc_trace.h"

#include <exception>
#include <warthog/domain/gridmap.h>
#include <warthog/search/problem_instance.h>
#include <warthog/search/search_node.h>

namespace warthog::io
{

/// @brief class that produces a posthoc trace for the gridmap domain, grid
/// must be set.
class grid_trace : public posthoc_trace
{
public:
	using node = search::search_node;

	grid_trace() = default;
	grid_trace(domain::gridmap* grid) : grid_(grid) { }

	void
	set_grid(domain::gridmap* grid) noexcept
	{
		grid_ = grid;
	}

	void
	print_posthoc_header() override;

	void
	begin_search(int id, const search::search_problem_instance& pi);

	void
	expand_node(const node& current) const;

	void
	relax_node(const node& current) const;

	void
	generate_node(
	    const node* parent, const node& child, cost_t edge_cost,
	    uint32_t edge_id) const;

	void
	close_node(const node& current) const;

protected:
	domain::gridmap* grid_;
};

} // namespace warthog::io

#endif // WARTHOG_IO_GRID_TRACE_H
