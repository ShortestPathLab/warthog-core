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

namespace warthog::graph
{

constexpr size_t graph_int8 = std::numeric_limits<uint8_t>::max();
constexpr size_t graph_int16 = std::numeric_limits<uint16_t>::max();
constexpr size_t graph_int32 = std::numeric_limits<uint32_t>::max();

enum edge_val : uint8_t
{
	e_OUT = 1U << 0,
	e_IN = 1U << 1,
	e_BOTH = e_OUT | e_IN,
};

template <typename edge_id>
struct vertex_edges
{
	edge_id start;
	edge_id count;
};
template <uint8_t dir, typename edge_id>
struct vertex_node;
template <typename edge_id>
struct vertex_node<e_OUT, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t edir() noexcept { return e_OUT; } 
	edge_type out;
};
template <typename edge_id>
struct vertex_node<e_IN, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t edir() noexcept { return e_IN; } 
	edge_type in;
};
template <typename edge_id>
struct vertex_node<e_BOTH, edge_id>
{
	using edge_type = vertex_edges<edge_id>;
	static consteval uint8_t edir() noexcept { return e_BOTH; } 
	edge_type out;
	edge_type in;
};

template <uint8_t dir, typename T>
	requires ((T::edir() & dir) == dir)
constexpr auto& vertex_node_edges(T& node) noexcept
{
	static_assert((dir & e_BOTH) != e_BOTH && (dir & e_BOTH) != 0);
	if constexpr ((dir & e_OUT)) {
		return node.out;
	} else {
		return node.in;
	}
}

struct graph_stats
{
	uint32_t vertices;
	uint32_t edges;

	uint32_t max_out_degree;
	uint32_t max_in_degree;
};

template <typename Graph>
graph_stats compute_stats(const Graph& G)
{
	graph_stats res;

	res.vertices = G.vertices();
	res.edges = G.edges();

	constexpr bool out = (Graph::graph_dir() & e_OUT) != 0;
	constexpr bool in = (Graph::graph_dir() & e_OUT) != 0;
	res.max_out_degree = 0;
	res.max_in_degree = 0;

	for (const auto& v : vertex_range(G)) {
		if constexpr (out) {
			uint32_t c = static_cast<uint32_t>(size(edge_range<e_OUT>(G, v)));
			res.max_out_degree = std::max(res.max_out_degree, c);
		}
		if constexpr (in) {
			uint32_t c = static_cast<uint32_t>(size(edge_range<e_IN>(G, v)));
			res.max_out_degree = std::max(res.max_in_degree, c);
		}
	}

	return res;
}

} // namespace warthog::graph

#endif // WARTHOG_DOMAIN_ADJ_GRAPH_H
