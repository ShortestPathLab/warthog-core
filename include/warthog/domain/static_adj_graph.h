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

#include <bit>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <warthog/constants.h>

namespace warthog::domain
{

namespace details
{

template <size_t MaxV, size_t MaxE, typename Edge>
struct static_adj_graph_data;



};

template <size_t MaxV, size_t MaxE, typename Edge = void, typename Factory = memory::alloc::malloc_factory>
class static_adj_graph
{

};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_ADJ_GRAPH_H
