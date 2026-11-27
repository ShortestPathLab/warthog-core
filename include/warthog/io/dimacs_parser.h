#ifndef WARTHOG_IO_DIMACS_PARSER_H
#define WARTHOG_IO_DIMACS_PARSER_H

/// @file dimacs_parser.h
///
/// A parser for reading road networks of the type
/// used at the 9th DIMACS competition.
///
/// @author dharabor
/// @date 2015-01-08

#include <warthog/forward.h>

#include "serialize_base.h"
#include <warthog/domain/static_adj_graph.h>
#include <warthog/search/problem_instance.h>

#include <cassert>
#include <fstream>
#include <ranges>
#include <span>
#include <vector>

namespace warthog::io
{

class dimacs_parser
{
public:
	struct node
	{
		uint32_t id_;
		int32_t x_;
		int32_t y_;
	};

	struct edge
	{
		uint32_t tail_id_;
		uint32_t head_id_;
		uint32_t weight_;

		template <graph::edge_val D>
		uint32_t end_id() const noexcept
		{
			static_assert(D == graph::e_in || D == graph::e_out);
			if constexpr (D == graph::e_in) {
				return head_id_;
			} else {
				return tail_id_;
			}
		}
		template <graph::edge_val D>
		uint32_t& end_id() noexcept
		{
			static_assert(D == graph::e_in || D == graph::e_out);
			if constexpr (D == graph::e_in) {
				return head_id_;
			} else {
				return tail_id_;
			}
		}

		bool
		operator<(dimacs_parser::edge& other)
		{
			return head_id_ < other.head_id_;
		}
	};

	struct experiment
	{
		uint32_t source;
		uint32_t target;
		bool p2p; // true indicates point-to-point instance; else SSSP

		search::problem_instance
		get_instance()
		{
			return search::problem_instance(pack_id{source}, pack_id{target});
		}
	};

	typedef std::vector<dimacs_parser::node>::iterator node_iterator;
	typedef std::vector<dimacs_parser::edge>::iterator edge_iterator;
	typedef std::vector<dimacs_parser::experiment>::iterator
	    experiment_iterator;

	dimacs_parser();
	dimacs_parser(const char* gr_file);
	dimacs_parser(const char* co_file, const char* gr_file);
	~dimacs_parser();

	// load up a DIMACS file (gr or co)
	// NB: upon invocation, this operation will discard all current nodes
	// (or edges, depending on the type of file passed for loading) and
	// THEN attempt to load new data.
	bool
	load_graph(std::string_view dimacs_file);

	bool
	load_instance(std::string_view dimacs_file);

	dimacs_parser::node&
	get_node(uint32_t id);

	dimacs_parser::node&
	get_dimacs_node(uint32_t id);

	inline uint32_t
	get_num_nodes()
	{
		return (uint32_t)nodes_.size();
	}

	inline uint32_t
	get_num_edges()
	{
		return (uint32_t)edges_.size();
	}

	std::span<const dimacs_parser::node>
	nodes() const noexcept
	{
		return nodes_;
	}
	std::span<dimacs_parser::node>
	nodes_mut() noexcept
	{
		return nodes_;
	}

	std::span<const dimacs_parser::edge>
	edges() const noexcept
	{
		return edges_;
	}
	std::span<dimacs_parser::edge>
	edges_mut() noexcept
	{
		return edges_;
	}

	std::span<const dimacs_parser::experiment>
	experiments() const noexcept
	{
		return experiments_;
	}
	std::span<dimacs_parser::experiment>
	experiments_mut() noexcept
	{
		return experiments_;
	}

	inline uint32_t
	num_experiments()
	{
		return (uint32_t)experiments_.size();
	}

	void
	print(std::ostream&);

	// convert to the graph format used by the METIS
	// graph partitioning library
	void
	print_undirected_unweighted_metis(
	    std::ostream&, double core_pct_value = 1,
	    std::vector<uint32_t>* lex_order = 0);

	inline std::string
	get_problemfile()
	{
		return problemfile_;
	}

	inline std::string
	get_co_filename()
	{
		return co_file_;
	}

	inline std::string
	get_gr_filename()
	{
		return gr_file_;
	}

private:

	void
	init();
	bool
	load_co_file(line_serialize& lines);
	bool
	load_gr_file(line_serialize& lines);

	std::vector<dimacs_parser::node> nodes_;
	std::vector<dimacs_parser::edge> edges_;
	std::vector<dimacs_parser::experiment> experiments_;
	std::string problemfile_;
	std::string co_file_;
	std::string gr_file_;
};


/// TODO: need features from other branches, help with control over Label and assigning to in dimics_to_graph

template<
    graph::edge_val GraphDir, size_t MaxV, size_t MaxE, typename LabelVertex, typename LabelOut, typename LabelIn>
    requires((GraphDir & graph::e_out) != 0)
void
dimics_to_graph(
    const dimacs_parser& dimic,
    domain::static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>& graph)
{
	using graph_type
	    = domain::static_adj_graph<GraphDir, MaxV, MaxE, LabelVertex, LabelOut, LabelIn>;
	auto nodes = dimic.nodes();
	// edges copied to enable sorting
	std::vector edges(dimic.edges().begin(), dimic.edges().end());
	graph.setup(nodes.size(), edges.size());

	// setup edges
	auto nd_vtx      = graph.vertex_all_mut();
	uint32_t edge_at = 0;

	// insert either e_out or e_in edges based on template
	auto edge_insert = [&]<graph::edge_val E>(std::integral_constant<graph::edge_val, E>)
	{
		constexpr auto Ei = invert_edge(E);
		using L = std::conditional_t<E == graph::e_in, LabelIn, LabelOut>;
		std::ranges::sort(
			edges, {}, [](const auto& e) noexcept { return e.template end_id<Ei>(); });
		for(auto const node_edges : edges
				| std::views::chunk_by([](const auto& a, const auto& b) noexcept {
										return a.template end_id<Ei>() == b.template end_id<Ei>();
									}))
		{
			uint32_t node_id = begin(node_edges)->template end_id<Ei>() - 1;
			// any skipped node will have zero edges
			auto& vtx_ed = vertex_node_edges<graph::e_out>(nd_vtx[node_id]);
			uint32_t ie  = size(node_edges);
			vtx_ed.start = static_cast<typename graph_type::edge_index>(edge_at);
			vtx_ed.count = ie;
			const auto vtx_st = vtx_ed;

			auto ed_to = edge_range_mut<E>(graph, vtx_st);
			for(uint32_t i = 0; i < ie; ++i)
			{
				ed_to[i] = static_cast<typename graph_type::vertex_index>(
						edges[edge_at + i].template end_id<E>() - 1);
				if constexpr(!std::same_as<L, void>)
				{
					if constexpr(std::assignable_from<L&, uint32_t>)
					{
						edge_range_mut<E, graph::label>(graph, vtx_st)[i]
							= edges[edge_at + i].weight_;
					}
					else
					{
						edge_range_mut<E, graph::label>(graph, vtx_st)[i]
							= static_cast<L>(edges[edge_at + i].weight_);
					}
				}
			}
			edge_at += ie;
		}
	};

	if constexpr (graph_type::graph_label_out()) {
		edge_insert(std::integral_constant<graph::edge_val, graph::e_out>());
	}
	if constexpr (graph_type::graph_label_in()) {
		edge_insert(std::integral_constant<graph::edge_val, graph::e_in>());
	}

	if constexpr (graph_type::graph_label_vertex()) {
		// add vertex labels if able
		using std::get;
		constexpr bool is_assign = requires (LabelVertex& v, uint32_t cost) {
			get<0>(v) = cost;
			get<1>(v) = cost;
		};
		if constexpr (is_assign)
		{
			for (auto&& [label,node] : std::views::zip(vertex_range_mut<graph::label>(graph), nodes)) {
				get<0>(label) = node.x_;
				get<1>(label) = node.y_;
			}
		}
	}
}

} // warthog::io

#endif // WARTHOG_IO_DIMACS_PARSER_H
