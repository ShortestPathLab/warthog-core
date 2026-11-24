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
	load_graph(const char* dimacs_file);

	bool
	load_instance(const char* dimacs_file);

	dimacs_parser::node&
	get_node(uint32_t id);

	dimacs_parser::node&
	get_dimacs_node(uint32_t id);

	inline uint32_t
	get_num_nodes()
	{
		return (uint32_t)nodes_->size();
	}

	inline uint32_t
	get_num_edges()
	{
		return (uint32_t)edges_->size();
	}

	std::span<const dimacs_parser::node>
	nodes() const noexcept
	{
		assert(nodes_ != nullptr);
		return *nodes_;
	}
	std::span<dimacs_parser::node>
	nodes_mut() noexcept
	{
		assert(nodes_ != nullptr);
		return *nodes_;
	}

	std::span<const dimacs_parser::edge>
	edges() const noexcept
	{
		assert(edges_ != nullptr);
		return *edges_;
	}
	std::span<dimacs_parser::edge>
	edges_mut() noexcept
	{
		assert(edges_ != nullptr);
		return *edges_;
	}

	std::span<const dimacs_parser::experiment>
	experiments() const noexcept
	{
		assert(experiments_ != nullptr);
		return *experiments_;
	}
	std::span<dimacs_parser::experiment>
	experiments_mut() noexcept
	{
		assert(experiments_ != nullptr);
		return *experiments_;
	}

	inline uint32_t
	num_experiments()
	{
		return (uint32_t)experiments_->size();
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
	load_co_file(std::istream& fdimacs);
	bool
	load_gr_file(std::istream& fdimacs);

	std::vector<dimacs_parser::node>* nodes_;
	std::vector<dimacs_parser::edge>* edges_;
	std::vector<dimacs_parser::experiment>* experiments_;
	std::string problemfile_;
	std::string co_file_;
	std::string gr_file_;
};

template<
    uint8_t GraphDir, size_t MaxV, size_t MaxE, typename CostOut,
    typename CostIn>
    requires((GraphDir & graph::e_out) != 0)
void
dimics_to_graph(
    const dimacs_parser& dimic,
    domain::static_adj_graph<GraphDir, MaxV, MaxE, CostOut, CostIn>& graph)
{
	using graph_type
	    = domain::static_adj_graph<GraphDir, MaxV, MaxE, CostOut, CostIn>;
	auto nodes = dimic.nodes();
	// copy edges, will sort
	std::vector edges(dimic.edges().begin(), dimic.edges().end());
	graph.setup(nodes.size(), edges.size());

	auto nd_vtx      = graph.vertex_all_mut();
	uint32_t edge_at = 0;
	std::ranges::sort(
	    edges, {}, [](const auto& e) noexcept { return e.head_id_; });
	for(auto const node_edges : edges
	        | std::views::chunk_by([](const auto& a, const auto& b) noexcept {
		                            return a.head_id_ == b.head_id_;
	                            }))
	{
		uint32_t node_id = begin(node_edges)->head_id_ - 1;
		// any skipped node will have zero edges
		auto& vtx_ed = vertex_node_edges<graph::e_out>(nd_vtx[node_id]);
		uint32_t ie  = size(node_edges);
		vtx_ed.start = edge_at;
		vtx_ed.count = ie;
		for(uint32_t i = 0; i < ie; ++i)
		{
			graph.edge_out.data[edge_at + i]
			    = static_cast<typename graph_type::vertex_id>(
			        edges[edge_at + i].tail_id_ - 1);
			if constexpr(!std::same_as<CostOut, void>)
			{
				if constexpr(std::assignable_from<CostOut&, uint32_t>)
				{
					graph.cost_out.data[edge_at + i]
					    = edges[edge_at + i].weight_;
				}
				else
				{
					graph.cost_out.data[edge_at + i]
					    = static_cast<CostOut>(edges[edge_at + i].weight_);
				}
			}
		}
		edge_at += ie;
	}

	if constexpr((GraphDir & graph::e_in) != 0)
	{
		// add in-edges
		std::ranges::sort(
		    edges, {}, [](const auto& e) noexcept { return e.tail_id_; });

		edge_at = 0;
		for(auto const node_edges : edges
		        | std::views::chunk_by([](const auto& a,
		                                  const auto& b) noexcept {
			                            return a.tail_id_ == b.tail_id_;
		                            }))
		{
			uint32_t node_id = begin(node_edges)->tail_id_ - 1;
			// any skipped node will have zero edges
			auto& vtx_ed = vertex_node_edges<graph::e_in>(nd_vtx[node_id]);
			uint32_t ie  = size(node_edges);
			vtx_ed.start = edge_at;
			vtx_ed.count = ie;
			for(uint32_t i = 0; i < ie; ++i)
			{
				graph.edge_in.data[edge_at + i]
				    = static_cast<typename graph_type::vertex_id>(
				        edges[edge_at + i].head_id_ - 1);
				if constexpr(!std::same_as<CostIn, void>)
				{
					if constexpr(std::assignable_from<CostIn&, uint32_t>)
					{
						graph.cost_in.data[edge_at + i]
						    = edges[edge_at + i].weight_;
					}
					else
					{
						graph.cost_in.data[edge_at + i]
						    = static_cast<CostIn>(edges[edge_at + i].weight_);
					}
				}
			}
			edge_at += ie;
		}
	}
}

} // warthog::io

#endif // WARTHOG_IO_DIMACS_PARSER_H
