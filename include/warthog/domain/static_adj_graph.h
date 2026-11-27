#ifndef WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H
#define WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H

/// @file domains/adj_graph.h
///
/// A generic directed static adjacency graph.
/// Requires knowning number of vertices and edges in advanced.
/// Supports flexiblity in design, including:
/// - 
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

#include "graph.h"
#include <warthog/util/numeric.h>

#include <limits>
#include <memory>
#include <span>

namespace warthog::domain
{

// forward declarations
namespace details {
template<
    graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
struct static_adj_graph_data;
} // namespace details

/// @brief efficent compact graph structure that does not support easy modifcation
/// @tparam LabelVertex data associated to vertices, void does not create data
/// @tparam LabelOut data associated with outbound edges, void does not create data
/// @tparam LabelIn data associated with inbound edges, void does not create data
/// @tparam GraphDir supported edge direction on vertices, e_out, e_in or e_both
/// @tparam MaxV maximum number of vertices supported, resizes uint to fit
/// @tparam MaxE maximum number of edges over all vertices supported, resizes uint to fit
///
/// A compact graph structure for storing a (weighted) directed graph.
/// The weighted component is defined by the user-provided Label's (can just be double).
///
/// The internal layout works by storeing all edge data isingle array,
/// storing all vertex data 
/// setup(v,e) -> v vertices, e edges
///    
template<
    graph::edge_val GraphDir = graph::e_out, size_t MaxV = graph::graph_int32,
    size_t MaxE = graph::graph_int16, typename LabelVertex = void, typename LabelOut = void,
    typename LabelIn = void>
class static_adj_graph : public details::static_adj_graph_data<
                             GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>
{
public:
	using super = details::static_adj_graph_data<
	    GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>;
	using label_vertex_type = LabelVertex;
	using label_out_type = LabelOut;
	using label_in_type  = LabelIn;
	using typename super::edge_id;
	using typename super::vertex_id;
	using typename super::edge_index;
	using typename super::vertex_index;
	using typename super::vertex_type;
	using typename super::vertex_edges_type;

	std::pair<uint32_t, uint32_t> graph_size; ///< first=num vertex, second=num edge

	static consteval uint8_t
	graph_dir() noexcept
	{
		return GraphDir;
	}
	static consteval bool
	graph_out() noexcept
	{
		return GraphDir & graph::e_out;
	}
	static consteval bool
	graph_in() noexcept
	{
		return GraphDir & graph::e_in;
	}
	static consteval bool
	graph_label_vertex() noexcept
	{
		return !std::same_as<LabelVertex, void>;
	}
	static consteval bool
	graph_label_out() noexcept
	{
		return !std::same_as<LabelOut, void>;
	}
	static consteval bool
	graph_label_in() noexcept
	{
		return !std::same_as<LabelIn, void>;
	}

	std::pair<uint32_t, uint32_t>
	sizes() const noexcept
	{
		return graph_size;
	}
	uint32_t
	vertices() const noexcept
	{
		return graph_size.first;
	}
	uint32_t
	edges() const noexcept
	{
		return graph_size.second;
	}

	void
	setup(uint32_t vcount, uint32_t ecount)
	{
		super::setup_(vcount, ecount);
		graph_size.first  = static_cast<uint32_t>(vcount);
		graph_size.second = static_cast<uint32_t>(ecount);
	}

	const vertex_type&
	vertex(uint32_t i) const noexcept
	{
		assert(i < this->vertices() && this->vertex_node != nullptr);
		return this->vertex_node[i];
	}
	vertex_type&
	vertex_mut(uint32_t i) noexcept
	{
		assert(i < this->vertices() && this->vertex_node != nullptr);
		return this->vertex_node[i];
	}

	std::span<const vertex_type>
	vertex_all() const noexcept
	{
		return std::span<const vertex_type>(
		    this->vertex_node.get(), this->vertices());
	}
	std::span<vertex_type>
	vertex_all_mut() noexcept
	{
		return std::span<vertex_type>(
		    this->vertex_node.get(), this->vertices());
	}

	const label_vertex_type&
	label_vertex_id(uint32_t i) const noexcept
	{
		assert(i < this->vertices() && this->label_vertex != nullptr);
		return this->label_vertex[i];
	}
	label_vertex_type&
	label_vertex_id_mut(uint32_t i) noexcept
	{
		assert(i < this->vertices() && this->label_vertex != nullptr);
		return this->label_vertex[i];
	}

	std::span<const label_vertex_type>
	label_vertex_all() const noexcept requires(graph_label_vertex())
	{
		return std::span<const label_vertex_type>(
		    this->label_vertex.get(), this->vertices());
	}
	std::span<label_vertex_type>
	label_vertex_all_mut() noexcept requires(graph_label_vertex())
	{
		return std::span<label_vertex_type>(
		    this->label_vertex.get(), this->vertices());
	}

#define WARTHOG_CODE_GENERATOR_STGRAPH_EDGE(name, type, varname, req, edir)   \
	std::span<const type> name##_all() const noexcept                         \
	    requires req                                                          \
	{                                                                         \
		return std::span<const type>(this->varname.get(), this->edges());     \
	}                                                                         \
	std::span<type> name##_all_mut() noexcept                                 \
	    requires req                                                          \
	{                                                                         \
		return std::span<type>(this->varname.get(), this->edges());           \
	}                                                                         \
	std::span<const type> name##_id(vertex_edges_type et) const noexcept      \
	    requires req                                                          \
	{                                                                         \
		assert((uint64_t)et.start.id + et.count <= this->edges());                         \
		return std::span<const type>(                                         \
		    this->varname.get() + et.start.id, et.count);                        \
	}                                                                         \
	std::span<type> name##_id_mut(vertex_edges_type et) noexcept              \
	    requires req                                                          \
	{                                                                         \
		assert((uint64_t)et.start.id + et.count <= this->edges());                         \
		return std::span<type>(this->varname.get() + et.start.id, et.count);     \
	}                                                                         \
	std::span<const type> name##_id(const vertex_type& vert) const noexcept   \
	    requires req                                                          \
	{                                                                         \
		return name##_id(vertex_node_edges<edir>(vert));                      \
	}                                                                         \
	std::span<type> name##_id_mut(const vertex_type& vert) noexcept           \
	    requires req                                                          \
	{                                                                         \
		return name##_id_mut(vertex_node_edges<edir>(vert));                  \
	}                                                                         \
	std::span<const type> name##_id(uint32_t vertex_id) const noexcept        \
	    requires req                                                          \
	{                                                                         \
		assert(vertex_id < this->vertices());                                 \
		return name##_id(                                                     \
		    vertex_node_edges<edir>(this->vertex_node[vertex_id]));           \
	}                                                                         \
	std::span<type> name##_id_mut(uint32_t vertex_id) noexcept                \
	    requires req                                                          \
	{                                                                         \
		assert(vertex_id < this->vertices());                                 \
		return name##_id_mut(                                                 \
		    vertex_node_edges<edir>(this->vertex_node[vertex_id]));           \
	}

	WARTHOG_CODE_GENERATOR_STGRAPH_EDGE(
	    edge_out, vertex_index, edge_out, (graph_out()), graph::e_out)
	WARTHOG_CODE_GENERATOR_STGRAPH_EDGE(
	    edge_in, vertex_index, edge_in, (graph_in()), graph::e_in)
	WARTHOG_CODE_GENERATOR_STGRAPH_EDGE(
	    label_out, LabelOut, label_out, (graph_label_out()), graph::e_out)
	WARTHOG_CODE_GENERATOR_STGRAPH_EDGE(
	    label_in, LabelIn, label_in, (graph_label_in()), graph::e_in)

#undef WARTHOG_CODE_GENERATOR_STGRAPH_EDGE
};

//
// static_adj_graph aliases
//

template<
    graph::edge_val GraphDir = graph::e_out, typename LabelVertex = void, typename LabelOut = void,
    typename LabelIn = void>
using static_adj_graph16 = static_adj_graph<
    GraphDir, graph::graph_int16, graph::graph_int16, LabelVertex, LabelOut, LabelIn>;

template<
    graph::edge_val GraphDir = graph::e_out, typename LabelVertex = void, typename LabelOut = void,
    typename LabelIn = void>
using static_adj_graph32 = static_adj_graph<
    GraphDir, graph::graph_int32, graph::graph_int32, LabelVertex, LabelOut, LabelIn>;

//
// static_adj_graph vertex and edge accessors
//

template<
    graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
auto
vertex_range(const static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G)
{
	if constexpr (D_get == graph::linkage) {
		return G.vertex_all();
	} else if constexpr (D_get == graph::label) {
		// will fail if LabelVertex is void
		return G.label_vertex_all();
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}
template<
    graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
auto
vertex_range_mut(static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G)
{
	if constexpr (D_get == graph::linkage) {
		return G.vertex_all_mut();
	} else if constexpr (D_get == graph::label) {
		// will fail if LabelVertex is void
		return G.label_vertex_all_mut();
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}

template<
    graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
const auto&
vertex_at(
    const static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G,
    uint32_t vertex_id)
{
	if constexpr (D_get == graph::linkage) {
		return G.vertex(vertex_id);
	} else if constexpr (D_get == graph::label) {
		// will fail if LabelVertex is void
		return G.label_vertex_id(vertex_id);
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}
template<
    graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
auto&
vertex_at_mut(
    static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G,
    uint32_t vertex_id)
{
	if constexpr (D_get == graph::linkage) {
		return G.vertex_mut(vertex_id);
	} else if constexpr (D_get == graph::label) {
		// will fail if LabelVertex is void
		return G.label_vertex_id_mut(vertex_id);
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}

template<
    graph::edge_val EdgeDir, graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
    requires((EdgeDir & GraphDir) == EdgeDir)
auto
edge_range(
    const static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G,
    const auto& vertex_identifier)
{
	static_assert(EdgeDir == graph::e_in || EdgeDir == graph::e_out, "EdgeDir musts be e_in or e_out");

	if constexpr (D_get == graph::linkage) {
		if constexpr(EdgeDir == graph::e_in)
		{
			return G.edge_in_id(vertex_identifier);
		}
		else if constexpr(EdgeDir == graph::e_out)
		{
			return G.edge_out_id(vertex_identifier);
		}
	} else if constexpr (D_get == graph::label) {
		if constexpr(EdgeDir == graph::e_in)
		{
			return G.label_in_id(vertex_identifier);
		}
		else if constexpr(EdgeDir == graph::e_out)
		{
			return G.label_out_id(vertex_identifier);
		}
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}
template<
    graph::edge_val EdgeDir, graph::data_level D_get = graph::linkage, graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
    requires((EdgeDir & GraphDir) == EdgeDir)
auto
edge_range_mut(
    static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& G,
    const auto& vertex_identifier)
{
	static_assert(EdgeDir == graph::e_in || EdgeDir == graph::e_out, "EdgeDir musts be e_in or e_out");

	if constexpr (D_get == graph::linkage) {
		if constexpr(EdgeDir == graph::e_in)
		{
			return G.edge_in_id_mut(vertex_identifier);
		}
		else if constexpr(EdgeDir == graph::e_out)
		{
			return G.edge_out_id_mut(vertex_identifier);
		}
	} else if constexpr (D_get == graph::label) {
		if constexpr(EdgeDir == graph::e_in)
		{
			return G.label_in_id_mut(vertex_identifier);
		}
		else if constexpr(EdgeDir == graph::e_out)
		{
			return G.label_out_id_mut(vertex_identifier);
		}
	} else {
		static_assert(false, "D_get must be linkage or label.");
	}
}

//
// details
//

namespace details
{

template<typename T>
struct static_adj_graph_label
{
	std::unique_ptr<T[]> data;

	void
	setup(size_t ecount)
	{
		data = std::make_unique<T[]>(ecount);
	}
	T*
	get() noexcept
	{
		return data.get();
	}
	const T*
	get() const noexcept
	{
		return data.get();
	}
};
template<>
struct static_adj_graph_label<void>
{
	void
	setup(size_t)
	{ }
};

template<
    graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
struct static_adj_graph_data
{
	static_assert(
	    std::same_as<LabelOut, void> || (GraphDir & graph::e_out) != 0);
	static_assert(std::same_as<LabelIn, void> || (GraphDir & graph::e_in) != 0);

	using vertex_id         = util::fit_unsigned_t<MaxV>;
	using edge_id           = util::fit_unsigned_t<MaxE>;
	using vertex_index      = graph::vertex_index<vertex_id>;
	using edge_index        = graph::edge_index<edge_id>;
	using vertex_type       = graph::vertex_node<GraphDir, edge_id>;
	using vertex_edges_type = typename vertex_type::edge_type;

	std::unique_ptr<vertex_type[]> vertex_node; ///< array of vertices of size vertices()
	[[no_unique_address]] static_adj_graph_label<
	    std::conditional_t<(GraphDir & graph::e_out) != 0, vertex_index, void>>
	    edge_out; ///< array of edge to of size edges(); only in e_out graphs
	[[no_unique_address]] static_adj_graph_label<
	    std::conditional_t<(GraphDir & graph::e_in) != 0, vertex_index, void>>
	    edge_in; ///< array of edge from of size edges(); only in e_in graphs
	[[no_unique_address]] static_adj_graph_label<LabelVertex> label_vertex; ///< array of vertex labels of size vertices(); only if not void
	[[no_unique_address]] static_adj_graph_label<LabelOut> label_out; ///< array of edge out labels of size edges(); only if not void
	[[no_unique_address]] static_adj_graph_label<LabelIn> label_in; ///< array of edge in labels of size edges(); only if not void

	void
	setup_(uint32_t vcount, uint32_t ecount)
	{
		vertex_node = std::make_unique<vertex_type[]>(vcount);
		edge_out.setup(ecount);
		edge_in.setup(ecount);
		label_vertex.setup(vcount);
		label_out.setup(ecount);
		label_in.setup(ecount);
	}
};

};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_STATIC_ADJ_GRAPH_H
