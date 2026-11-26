#ifndef WARTHOG_DOMAIN_GRAPH_H
#define WARTHOG_DOMAIN_GRAPH_H

/// @file adj_graph.h
///
/// Gereric graph features.
///
/// @author Ryan Hechenberger
/// @date 2026-11-11

#include <warthog/forward.h>

#include <warthog/util/numeric.h>

#include <limits>
#include <memory>
#include <cassert>

namespace warthog::graph
{

constexpr size_t graph_int8  = std::numeric_limits<uint8_t>::max();
constexpr size_t graph_int16 = std::numeric_limits<uint16_t>::max();
constexpr size_t graph_int32 = std::numeric_limits<uint32_t>::max();

enum edge_val : uint8_t
{
	e_out  = 1U << 0,
	e_in   = 1U << 1,
	e_both = e_out | e_in,
};

constexpr edge_val invert_edge(edge_val a) noexcept
{
	assert(a == e_out || a == e_in);
	return static_cast<edge_val>(a ^ e_both);
}

template<typename edge_id>
struct vertex_edges
{
	edge_id start;
	edge_id count;
};
template<uint8_t dir, typename edge_id>
struct vertex_node;
template<typename edge_id>
struct vertex_node<e_out, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t
	edir() noexcept
	{
		return e_out;
	}
	edge_type out;
};
template<typename edge_id>
struct vertex_node<e_in, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t
	edir() noexcept
	{
		return e_in;
	}
	edge_type in;
};
template<typename edge_id>
struct vertex_node<e_both, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t
	edir() noexcept
	{
		return e_both;
	}
	edge_type out;
	edge_type in;
};

template<uint8_t dir, typename T>
    requires((T::edir() & dir) == dir)
constexpr auto&
vertex_node_edges(T& node) noexcept
{
	static_assert((dir & e_both) != e_both && (dir & e_both) != 0);
	if constexpr((dir & e_out)) { return node.out; }
	else { return node.in; }
}

struct graph_stats
{
	uint32_t vertices;
	uint32_t edges;

	uint32_t max_out_degree;
	uint32_t max_in_degree;
};

template<typename Graph>
graph_stats
compute_stats(const Graph& G)
{
	graph_stats res;

	res.vertices = G.vertices();
	res.edges    = G.edges();

	constexpr bool out = (Graph::graph_dir() & e_out) != 0;
	constexpr bool in  = (Graph::graph_dir() & e_in) != 0;
	res.max_out_degree = 0;
	res.max_in_degree  = 0;

	for(const auto& v : vertex_range(G))
	{
		if constexpr(out)
		{
			uint32_t c = static_cast<uint32_t>(size(edge_range<e_out>(G, v)));
			res.max_out_degree = std::max(res.max_out_degree, c);
		}
		if constexpr(in)
		{
			uint32_t c = static_cast<uint32_t>(size(edge_range<e_in>(G, v)));
			res.max_out_degree = std::max(res.max_in_degree, c);
		}
	}

	return res;
}

} // namespace warthog::graph

#endif // WARTHOG_DOMAIN_ADJ_GRAPH_H
