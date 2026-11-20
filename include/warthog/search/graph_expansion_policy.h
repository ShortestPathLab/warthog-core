#ifndef WARTHOG_SEARCH_GRAPH_EXPANSION_POLICY_H
#define WARTHOG_SEARCH_GRAPH_EXPANSION_POLICY_H

/// @file graph_expansion_policy.h
///
/// An ExpansionPolicy for generic graphs.
/// Supports a generic graph class that uses accessor functions
/// as outlined in <warthog/domain/graph.h>.
///
/// @author Ryan Hechenberger
/// @date 2026-11-20

#include <warthog/forward.h>

#include "expansion_policy.h"
#include "problem_instance.h"
#include "search_node.h"
#include <warthog/domain/graph.h>

#include <memory>

namespace warthog::search
{

struct get_graph_cost_cast
{
	cost_t operator()(const auto& G, const auto& edge_data)
	{
		return static_cast<cost_t>(edge_data);
	}
};
struct get_graph_cost_unit
{
	constexpr cost_t operator()() noexcept
	{
		return 1;
	}
};

template <typename G>
class graph_expansion_policy_base : public expansion_policy
{
public:
	graph_expansion_policy_base(G* map);

	void
	set_map(G& map);

	search_problem_instance
	get_problem_instance(problem_instance* pi) override;

	pack_id
	get_state(pad_id node_id) override
	{
		return static_cast<pack_id>(node_id);
	}
	pad_id
	unget_state(pack_id node_id) override
	{
		return static_cast<pack_id>(node_id);
	}

	G*
	get_map() const noexcept
	{
		return map_;
	}

	void
	print_node(search_node* n, std::ostream& out) override;

	size_t
	mem() override;

protected:
	G* map_ = nullptr;
};

template <typename G, typename CostConv>
class graph_expansion_policy : public graph_expansion_policy_base<G>
{
public:
	graph_expansion_policy(G* map, CostConv&& conv = CostConv()) :
		graph_expansion_policy_base(map), cost_conv(std::move(conv))
	{ }

	void
	expand(search_node*, search_problem_instance*) override;

	search_node*
	generate_start_node(search_problem_instance* pi) override;

	search_node*
	generate_target_node(search_problem_instance* pi) override;

	size_t
	mem() override
	{
		return 0;
	}

protected:
	[[no_unqiue_address]] cost_conv;
};

template <typename G, typename CostConv>
void
graph_expansion_policy<G, CostConv>::expand(search_node* current, search_problem_instance* problem)
{
	reset();

	uint32_t cid = static_cast<uint32_t>(current->id);

	if constexpr (std::invocable<CostConv&>) {
		// no data
		for (auto i : edge_range(*this->map_, cid)) {
			add_neighbour(this->generate(i), cost_conv());
		}
	} else {
		auto& map = *this->map_;
		for (auto i : std::views::zip(edge_range(map, cid), cost_range(map, cid))) {
			add_neighbour(this->generate(pad_id(std::get<0>(i))), cost_conv(map, std::get<1>(i)));
		}
	}
}

template <typename G, typename CostConv>
search_node*
graph_expansion_policy<G, CostConv>::generate_start_node(search_problem_instance* pi)
{
	uint32_t max_id = this->map_->vertices();
	if(uint32_t{pi->start_} >= max_id) { return 0; }
	return generate(pi->start_);
}

template <typename G, typename CostConv>
search_node*
graph_expansion_policy<G, CostConv>::generate_target_node(search_problem_instance* pi)
{
	uint32_t max_id = this->map_->vertices();
	if(uint32_t{pi->target_} >= max_id) { return 0; }
	return generate(pi->target_);
}


} // namespace warthog::search

#endif // WARTHOG_SEARCH_GRAPH_EXPANSION_POLICY_H
