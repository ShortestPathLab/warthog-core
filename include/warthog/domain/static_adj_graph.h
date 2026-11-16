#ifndef WARTHOG_DOMAIN_ADJ_GRAPH_H
#define WARTHOG_DOMAIN_ADJ_GRAPH_H

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

template <uint8_t ID, typename T>
struct static_adj_graph_edges
{
	std::unique_ptr<EdgeData[]> data;
	
	void setup(size_t ecount)
	{
		data = std::make_unique<EdgeData[]>(ecount);
	}
};

template <uint8_t ID, typename T, typename edge_id>
std::span<const T> get_vertex_data(const static_adj_graph_edges<ID, T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<const T>(edges.data.get() + ve.start, ve.count);
}
template <uint8_t ID, typename T, typename edge_id>
std::span<T> get_vertex_data_mut(static_adj_graph_edges<ID, T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<T>(edges.data.get() + ve.start, ve.count);
}

template <uint8_t ID, typename T, typename edge_id>
std::span<const T> get_vertex_data(const static_adj_graph_edges<ID, T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<const T>(edges.data.get() + ve.start, ve.count);
}
template <uint8_t ID, typename T, typename edge_id>
std::span<T> get_vertex_data_mut(static_adj_graph_edges<ID, T>& edges, graph::vertex_edges<edge_id> ve)
{
	return std::span<T>(edges.data.get() + ve.start, ve.count);
}

template <size_t MaxV, size_t MaxE, typename EdgeData>
struct static_adj_graph_data : static_adj_graph_data<MaxV, MaxE, void>
{
	using super = static_adj_graph_data<MaxV, MaxE, void>;
	using typename super::vertex_id;
	using typename super::edge_id;
	using typename super::vertex_node;
	
	std::unique_ptr<EdgeData[]> Edata;

	void set_size(vertex_id vcount, edge_id ecount)
	{
		super::set_size(vcount, ecount);
		Edata = std::make_unique<EdgeData[]>(ecount);
	}

	std::span<const EdgeData> vertex_out_data(vertex_node node) const noexcept
	{
		assert((size_t)node.edge_start + (size_t)node.edge_count <= this->edge_count);
		return std::span(Edata.get() + node.edge_start, node.edge_count);
	}
	std::span<const EdgeData> vertex_out_data(vertex_id id) const noexcept
	{
		return vertex_out(vertex(id));
	}

	const EdgeData& edge_data(edge_id id)
	{
		assert(id < this->edge_count);
		return Edata[id];
	}
};
template <size_t MaxV, size_t MaxE>
struct static_adj_graph_data<MaxV, MaxE, void>
{
	using vertex_id = util::fit_unsigned_t<MaxV>;
	using edge_id = util::fit_unsigned_t<MaxV>;

	struct vertex_node {
		edge_id edge_start;
		edge_id edge_count;
	};

	vertex_id vertex_count;
	edge_id edge_count;
	std::unique_ptr<vertex_node[]> V;
	std::unique_ptr<vertex_id[]> E;

	void set_size(vertex_id vcount, edge_id ecount)
	{
		V = std::make_unique<vertex_node[]>(vcount);
		vertex_count = vcount;
		V = std::make_unique<vertex_id[]>(ecount);
		edge_count = ecount;
	}

	vertex_node vertex(vertex_id id) const noexcept
	{
		assert(id < vertex_count);
		return V[id];
	}

	std::span<const vertex_id> vertex_out(vertex_node node) const noexcept
	{
		assert((size_t)node.edge_start + (size_t)node.edge_count <= edge_count);
		return std::span(E.get() + node.edge_start, node.edge_count);
	}
	std::span<const vertex_id> vertex_out(vertex_id id) const noexcept
	{
		return vertex_out(vertex(id));
	}

	vertex_id edge(edge_id id) const noexcept
	{
		assert(id < edge_count);
		return V[id];
	}
};


};

template <size_t MaxV = graph_int16, size_t MaxE = graph_int16, typename EdgeData = void>
class static_adj_graph : public details::static_adj_graph_data<MaxV, MaxE, EdgeData>
{

};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_ADJ_GRAPH_H
