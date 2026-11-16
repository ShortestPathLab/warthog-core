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
	e_EXTRA1 = 1U << 2,
	e_EXTRA2 = 1U << 3,
	e_EXTRA3 = 1U << 4,
	e_EXTRA4 = 1U << 5,
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
	static constexpr uint8_t edir() noexcept { return dir; } 
	vertex_edges out;
};
template <typename edge_id>
struct vertex_node<e_IN, edge_id>
{
	static constexpr uint8_t edir() noexcept { return dir; } 
	vertex_edges in;
};
template <typename edge_id>
struct vertex_node<e_BOTH, edge_id>
{
	static constexpr uint8_t edir() noexcept { return dir; } 
	vertex_edges out;
	vertex_edges in;
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

} // namespace warthog::graph

#endif // WARTHOG_DOMAIN_ADJ_GRAPH_H
