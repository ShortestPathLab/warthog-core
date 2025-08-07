#ifndef WARTHOG_IO_GRID_TRACE_H
#define WARTHOG_IO_GRID_TRACE_H

// listener/grid_trace.h
//
// @author: Ryan Hechenberger
// @created: 2025-08-07
//

#include "posthoc_listener.h"
#include <warthog/search/search_node.h>
#include <warthog/search/problem_instance.h>
#include <warthog/domain/gridmap.h>
#include <exception>

namespace warthog::io
{

/// @brief class that produces a posthoc trace for the gridmap domain  
class grid_trace : public posthoc_listener
{
public:
	using node = search::search_node;
	using posthoc_listener::posthoc_listener;

	void set_grid(domain::gridmap* grid) noexcept
	{
		grid_ = grid;
	}

	void print_posthoc_header() override;

	/// @brief Checks that grid_ != nullptr
	void event(const char* name) const
	{
		if (grid_ == nullptr) {
			throw std::logic_error("grid_trace::grid_ is null");
		}
	}

	void begin_search(int id, const search::search_problem_instance& pi);

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
