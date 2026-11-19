#ifndef WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H
#define WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H

/// @file domains/adj_graph.h
///
/// A generic directed adjacency graph.
/// Supports compact and simple graph with flexiblity of design.
///
/// Two variants are presented:
/// - static_adj_graph as a graph that never changes, and known in advanced.
/// - dynamic_adj_graph for a graph that 
/// 
///
/// @author Ryan Hechenberger
/// @date 2026-11-04
///

/// NOTES: https://benchmarks.pathfinding.ai/2d-benchmarks/road-maps/ missing

#include <warthog/constants.h>

#include <warthog/util/numeric.h>
#include "graph.h"

#include <limits>
#include <memory>
#include <span>

namespace warthog::domain
{

namespace details
{

template <typename T>
struct static_adj_graph_edges
{
	std::unique_ptr<T[]> data;
	
	void setup(size_t ecount)
	{
		data = std::make_unique<T[]>(ecount);
	}
};
template <>
struct static_adj_graph_edges<void>
{
	void setup(size_t)
	{ }
};

template <typename T, typename edge_id>
std::span<const T> get_vertex_data(const static_adj_graph_edges<T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<const T>(edges.data.get() + ve.start, ve.count);
}
template <typename T, typename edge_id>
std::span<T> get_vertex_data_mut(static_adj_graph_edges<T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<T>(edges.data.get() + ve.start, ve.count);
}


template <uint8_t GraphDir, size_t MaxV, size_t MaxE, typename CostOut, typename CostIn>
struct static_adj_graph_data
{
	static_assert(std::same_as<CostOut,void> || (GraphDir&graph::e_OUT) != 0);
	static_assert(std::same_as<CostIn,void> || (GraphDir&graph::e_IN) != 0);
	using vertex_id = util::fit_unsigned_t<MaxV>;
	using edge_id = util::fit_unsigned_t<MaxE>;
	using vertex_type = graph::vertex_node<GraphDir, edge_id>;
	
	std::pair<uint32_t,uint32_t> graph_size;
	std::unique_ptr<vertex_type[]> vertex_node;
	[[no_unique_address]] static_adj_graph_edges<std::conditional_t<(GraphDir&graph::e_OUT)!=0, vertex_id, void>> edge_out;
	[[no_unique_address]] static_adj_graph_edges<std::conditional_t<(GraphDir&graph::e_OUT)!=0, vertex_id, void>> edge_in;
	[[no_unique_address]] static_adj_graph_edges<CostOut> cost_out;
	[[no_unique_address]] static_adj_graph_edges<CostIn> cost_in;

	void setup(vertex_id vcount, edge_id ecount)
	{
		graph_size.first = static_cast<uint32_t>(vcount);
		graph_size.second = static_cast<uint32_t>(ecount);
		vertex_node = std::make_unique<vertex_type[]>(vcount);
		std::fill_n(vertex_node.get(), vcount, vertex_type{});
		edge_out.setup(ecount);
		edge_in.setup(ecount);
		cost_out.setup(ecount);
		cost_in.setup(ecount);
	}

	std::pair<uint32_t,uint32_t> sizes() const noexcept
	{
		return graph_size;
	}
	uint32_t vertices() const noexcept
	{
		return graph_size.first;
	}
	uint32_t edges() const noexcept
	{
		return graph_size.second;
	}
};


};

template <uint8_t GraphDir = graph::e_OUT, size_t MaxV = graph::graph_int16, size_t MaxE = graph::graph_int16, typename CostOut = void, typename CostIn = void>
class static_adj_graph : public details::static_adj_graph_data<GraphDir, MaxV, MaxE, CostOut, CostIn>
{
public:
	using super = details::static_adj_graph_data<GraphDir, MaxV, MaxE, CostOut, CostIn>;
	using cost_out_type = CostOut;
	using cost_in_type = CostIn;
	using typename super::vertex_id;
	using typename super::edge_id;
	using typename super::vertex_type;
	using vertex_edges = graph::vertex_edges<edge_id>;

	const vertex_type& vertex(uint32_t i) const noexcept
	{
		assert(i < this->vertices() && this->vertex_node != nullptr);
		return this->vertex_node[i];
	}
	vertex_type& vertex_mut(uint32_t i) noexcept
	{
		assert(i < this->vertices() && this->vertex_node != nullptr);
		return this->vertex_node[i];
	}
	std::span<const vertex_type> vertex_span() const noexcept
	{
		return std::span<const vertex_type>(this->vertex_node.get(), this->vertices());
	}
	std::span<vertex_type> vertex_span_mut() noexcept
	{
		return std::span<vertex_type>(this->vertex_node.get(), this->vertices());
	}
};

template <uint8_t GraphDir = graph::e_OUT, typename CostOut = void, typename CostIn = void>
using static_adj_graph16 = static_adj_graph<GraphDir, graph::graph_int16, graph::graph_int16, CostOut, CostIn>;

template <uint8_t GraphDir = graph::e_OUT, typename CostOut = void, typename CostIn = void>
using static_adj_graph32 = static_adj_graph<GraphDir, graph::graph_int32, graph::graph_int32, CostOut, CostIn>;

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H
