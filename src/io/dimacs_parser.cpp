#include <warthog/io/dimacs_parser.h>

#include <warthog/constants.h>

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

namespace warthog::io
{

dimacs_parser::dimacs_parser()
{
	init();
}

dimacs_parser::dimacs_parser(const char* gr_file)
{
	init();
	load_graph(gr_file);
}

dimacs_parser::dimacs_parser(const char* co_file, const char* gr_file)
{
	init();
	load_graph(co_file);
	load_graph(gr_file);
}

dimacs_parser::~dimacs_parser() = default;

void
dimacs_parser::init()
{ }

dimacs_parser::node&
dimacs_parser::get_node(uint32_t id)
{
	return nodes_.at(id);
}

dimacs_parser::node&
dimacs_parser::get_dimacs_node(uint32_t id)
{
	return nodes_.at(id - 1);
}

bool
dimacs_parser::load_graph(std::string_view filename)
{
	using namespace std::string_view_literals;
	std::ifstream fdimacs{std::string(filename)};
	line_serialize lines;
	if (!lines.open_read(&fdimacs)) {
		std::cerr << "err; dimacs_parser::dimacs_parser "
		             "cannot open file: "
		          << filename << std::endl;
		return false;
	}

	bool retval       = true;
	while(true)
	{
		line_serialize::parser par;
		if (auto r = lines.readline(); r) {
			par.str(*r);
		} else {
			return false;
		}
		if (lines.istream_eof())
			break;

		std::string_view token;
		if (!par.next(token))
			return false;
		if(token.at(0) == 'c') continue;

		if(token.at(0) == 'p')
		{
			if (!par.next(token))
				return false;
			if(token == "sp"sv)
			{
				uint32_t tmp_num_nodes, tmp_num_edges;
				if (!par.next(tmp_num_nodes).next(tmp_num_edges))
					return false;
				if(tmp_num_edges == 0 || tmp_num_nodes == 0)
				{
					std::cerr << "error; invalid graph description on line "
					          << lines.get_line_num() << " of file " << filename << "\n";
					return false;
				}
				std::cerr << "loading " << tmp_num_edges << " arcs " << "from "
				          << filename << " ... ";
				edges_.reserve(tmp_num_edges);
				retval   = load_gr_file(lines);
				if (!retval)
					return false;
				gr_file_ = filename;
				assert(tmp_num_edges == get_num_edges());
				std::cerr << "done\n";
				break;
			}
			else if(token == "aux"sv)
			{
				if (!par.next(token)) return false;
				if(token != "sp"sv) { retval = false; }
				else
				{
					if (!par.next(token)) return false;
					if(token != "co"sv)
					{
						std::cerr
						    << "error; invalid graph description on line "
						    << lines.get_line_num() << " of file " << filename << "\n";
						return 0;
					}
					else
					{
						uint32_t tmp_num_nodes;
						if (!par.next(tmp_num_nodes))
							return false;
						if(tmp_num_nodes == 0)
						{
							std::cerr
							    << "error; invalid graph description on line "
							    << lines.get_line_num() << " of file " << filename << "\n";
							return 0;
						}
						std::cerr << "loading " << tmp_num_nodes << " nodes "
						          << "from " << filename << " ... ";
						nodes_.reserve(tmp_num_nodes);
						retval   = load_co_file(lines);
						gr_file_ = filename;
						assert(tmp_num_nodes == get_num_nodes());
						std::cerr << "done\n";
						break;
					}
				}
			}
			else
			{
				std::cerr
				    << "error; unrecognised problem line in dimacs file\n";
				return 0;
			}
		}
	}

	return retval;
}

bool
dimacs_parser::load_co_file(line_serialize& lines)
{
	using namespace std::string_view_literals;

	if (!lines.can_read())
		return false;
	nodes_.clear();
	bool all_good     = true;
	while(all_good)
	{
		line_serialize::parser par;
		if (auto r = lines.readline(); r) {
			par.str(*r);
		} else {
			return false;
		}
		if (lines.istream_eof())
			break;

		std::string_view token;
		if (!par.next(token) || token.size() != 1)
			return false;
		switch(token[0])
		{
		case 'v':
		{
			dimacs_parser::node n;
			if (!par.next(n.id_).next(n.x_).next(n.y_))
			{
				std::cerr
				    << "warning; badly formatted node descriptor on line "
				    << lines.get_line_num() << std::endl;
				all_good = false;
				break;
			}
			nodes_.push_back(n);
			break;
		}
		case 'p': // stop if we hit another problem line
			all_good = false;
			break;
		default: // ignore non-node, non-problem lines
			break;
		}
	}

	return all_good;
}

bool
dimacs_parser::load_gr_file(line_serialize& lines)
{
	using namespace std::string_view_literals;

	if (!lines.can_read())
		return false;
	bool all_good     = true;
	while(all_good)
	{
		line_serialize::parser par;
		std::string_view line;
		if (auto r = lines.readline(); r) {
			line = *r;
			par.str(*r);
		} else {
			return false;
		}
		if (lines.istream_eof()) {
			break;
		}
		
		std::string_view token;
		if (!par.next(token) || token.size() != 1)
			return false;
		switch(token[0])
		{
		case 'a':
		{
			dimacs_parser::edge e;
			if (!par.next(e.tail_id_).next(e.head_id_).next(e.weight_))
			{
				std::cerr << "warning; badly formatted arc descriptor on line "
				          << lines.get_line_num() << std::endl;
				all_good = false;
				break;
			}
			edges_.push_back(e);
			break;
		}
		case 'p': // another problem line. stop here
			all_good = false;
			break;
		default: // ignore non-arc, non-problem lines
			break;
		}
	}

	return all_good;
}

void
dimacs_parser::print(std::ostream& oss)
{
	uint32_t nnodes = (uint32_t)nodes_.size();
	if(nnodes > 0)
	{
		oss << "p aux sp co " << nodes_.size() << std::endl;
		for(uint32_t i = 0; i < nnodes; i++)
		{
			dimacs_parser::node n = nodes_.at(i);
			oss << "v " << i + 1 << " " << n.x_ << " " << n.y_ << std::endl;
		}
	}

	uint32_t nedges = (uint32_t)edges_.size();
	if(nedges > 0)
	{
		oss << "p sp " << nnodes << " " << nedges << std::endl;
		for(uint32_t i = 0; i < nedges; i++)
		{
			dimacs_parser::edge e = edges_.at(i);
			oss << "a " << e.tail_id_ << " " << e.head_id_ << " " << e.weight_
			    << std::endl;
		}
	}
}

bool
dimacs_parser::load_instance(std::string_view dimacs_file)
{
	using namespace std::string_view_literals;
	problemfile_ = std::string(dimacs_file);
	
	std::ifstream fdimacs{problemfile_};
	line_serialize lines;
	if (!lines.open_read(&fdimacs)) {
		std::cerr << "err; dimacs_parser::dimacs_parser "
		             "cannot open file: "
		          << problemfile_ << std::endl;
		return false;
	}

	bool p2p = true;

	// dimacs stuff is 1-indexed but warthog is zero indexed
	// we adjust ids if necessary
	uint32_t id_offset = 1;
	while(true)
	{
		line_serialize::parser par;
		std::string_view line;
		if (auto r = lines.readline(); r) {
			par.str(line = *r);
		} else {
			return false;
		}
		if (lines.istream_eof())
			return false;
		
		std::string_view token;
		if (!par.next(token))
			return false;
		// skip comment lines
		if(token == "c"sv) { continue; }
		// "p2p-xy is a warthog extended problem specification format
		// where start and target ids are zero indexed. In these cases
		// we don't need to adjust the start and target ids
		if(line.starts_with("p aux sp p2p-zero"sv))
		{
			p2p       = true;
			id_offset = 0;
			break;
		}
		if(line.starts_with("p aux sp ss-zero"))
		{
			id_offset = 0;
			p2p       = false;
			break;
		}
		if(line.starts_with("p aux sp p2p"))
		{
			p2p = true;
			break;
		}
		if(line.starts_with("p aux sp ss"))
		{
			p2p = false;
			break;
		}
	}

	while(true)
	{
		line_serialize::parser par;
		if (auto r = lines.readline(); r) {
			par.str(*r);
		} else {
			return false;
		}
		if (lines.istream_eof())
			break;

		std::string_view token;
		if (!par.next(token))
			return false;
		if(token == "c"sv)
			continue;

		if(token == "q"sv || token == "s"sv)
		{
			dimacs_parser::experiment exp;

			if (!par.next(exp.source))
			{
				std::cerr << "skipping invalid query in problem file on line " << lines.get_line_num()
				          << "\n";
				continue;
			}
			exp.source -= id_offset;
			exp.p2p     = p2p;

			if(p2p)
			{
				if (!par.next(exp.target))
				{
					std::cerr << "invalid query in problem file on line " << lines.get_line_num()
					          << "\n";
					return false;
				}
				exp.target -= id_offset;
			}
			else { exp.target = warthog::INF32; }

			// add the experiment
			experiments_.push_back(exp);
		}
		else
		{
			std::cerr << "skipping invalid query in problem file on line " << lines.get_line_num()
			          << std::endl;
		}
	}
	// std::cerr << "loaded "<<experiments_.size() << " queries\n";
	return true;
}

void
dimacs_parser::print_undirected_unweighted_metis(
    std::ostream& out, double core_pct_value, std::vector<uint32_t>* lex_order)
{
	std::unordered_map<uint32_t, std::set<uint32_t>> adj;
	std::set<uint32_t> nodes;

	uint32_t num_undirected_edges = 0;
	uint32_t id_offset = 1; // order ids begin from 0; dimacs ids from 1

	// enumerate all the nodes
	for(uint32_t i = 0; i < edges_.size(); i++)
	{
		dimacs_parser::edge e = edges_.at(i);
		uint32_t id1          = e.head_id_;
		uint32_t id2          = e.tail_id_;
		bool include_id1 = true, include_id2 = true;

		// filtering stuff for when the DIMACS graph is a CH and we want to
		// convert just a subset of the graph (== a core graph)
		if(lex_order)
		{
			// core is the top-k% of nodes;
			uint32_t min_core_level
			    = (uint32_t)((double)lex_order->size() * (1 - core_pct_value));

			// metis ids need to be contiguous and start from 1
			if(lex_order->at(id1 - id_offset) >= min_core_level)
			{
				id1 = id1 - min_core_level;
				assert(id1 < lex_order->size());
			}
			else { include_id1 = false; }

			if(lex_order->at(id2 - id_offset) >= min_core_level)
			{
				id2 = id2 - min_core_level;
				assert(id2 < lex_order->size());
			}
			else { include_id2 = false; }
		}

		if(include_id1) { nodes.insert(id1); }
		if(include_id2) { nodes.insert(id2); }

		if(adj.find(id1) == adj.end())
		{
			std::set<uint32_t> neis;
			std::pair<uint32_t, std::set<uint32_t>> elt(id1, neis);
			adj.insert(elt);
		}
		if(adj.find(id2) == adj.end())
		{
			std::set<uint32_t> neis;
			std::pair<uint32_t, std::set<uint32_t>> elt(id2, neis);
			adj.insert(elt);
		}

		if(include_id1 && include_id2)
		{
			std::set<uint32_t>& neis1 = adj.find(id1)->second;
			std::set<uint32_t>& neis2 = adj.find(id2)->second;
			if(neis1.find(id2) == neis1.end()
			   && neis2.find(id1) == neis2.end())
			{
				num_undirected_edges++;
			}

			auto iter1 = adj.find(id1);
			auto iter2 = adj.find(id2);
			assert(iter1 != adj.end() && iter2 != adj.end());
			iter1->second.insert(id2);
			iter2->second.insert(id1);
		}
	}
	std::cerr << "conversion done; " << nodes.size() << " nodes and "
	          << num_undirected_edges << " edges; printing\n";

	// ids need to be a contiguous series, from 1 to nodes.size()
	// here we convert the identifiers (since the input file may
	// not be dimacs compliant in this respect)
	std::unordered_map<uint32_t, uint32_t> id_map;
	uint32_t current_id = 1;
	for(std::set<uint32_t>::iterator it = nodes.begin(); it != nodes.end();
	    it++)
	{
		id_map.insert(std::pair<uint32_t, uint32_t>(*it, current_id));
		current_id++;
	}

	out << nodes.size() << " " << num_undirected_edges << std::endl;
	for(auto it = nodes.begin(); it != nodes.end(); it++)
	{
		std::set<uint32_t>& neis = adj.find(*it)->second;
		for(auto nit = neis.begin(); nit != neis.end(); nit++)
		{
			out << (*id_map.find((*nit))).second << " ";
		}
		out << std::endl;
	}
}

} // namespace warthog::io
