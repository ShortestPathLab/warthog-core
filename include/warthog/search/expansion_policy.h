#ifndef WARTHOG_SEARCH_EXPANSION_POLICY_H
#define WARTHOG_SEARCH_EXPANSION_POLICY_H

// expansion_policy.h
//
// An (abstract) expansion policy for searching explicit
// domains. It is assumed the number of nodes in the
// search space is known apriori and a description of each node can be
// generated in constant time and independent of any other node.
//
// @author: dharabor
// @created: 2016-01-26
//

#include "problem_instance.h"
#include "search_node.h"
#include <warthog/memory/arraylist.h>
#include <warthog/memory/node_pool.h>

#include <vector>

namespace warthog::search
{

class expansion_policy
{
public:
	expansion_policy(size_t nodes_pool_size);
	virtual ~expansion_policy();

	size_t
	get_nodes_pool_size()
	{
		return nodes_pool_size_;
	}

	inline void
	reclaim()
	{
		// reset();
		// nodepool_->eclaim();
	}

	inline void
	reset()
	{
		current_ = 0;
		neis_->clear();
	}

	inline void
	first(search_node*& ret, double& cost)
	{
		current_ = 0;
		n(ret, cost);
	}

	inline void
	n(search_node*& ret, double& cost)
	{
		if(current_ < neis_->size())
		{
			ret = (*neis_)[current_].node_;
			cost = (*neis_)[current_].cost_;
		}
		else
		{
			ret = 0;
			cost = 0;
		}
	}

	// return the nth successor
	// NB: also adjust the current neighbour index such that the
	// subsequent call to ::next will return the nth+1 neighbour.
	inline void
	get_successor(uint32_t which, search_node*& ret, double& cost)
	{
		if(which < neis_->size())
		{
			current_ = which;
			n(ret, cost);
		}
		else
		{
			ret = 0;
			cost = 0;
		}
	}

	inline void
	next(search_node*& ret, double& cost)
	{
		current_++;
		n(ret, cost);
	}

	inline size_t
	get_num_successors()
	{
		return neis_->size();
	}

	virtual size_t
	mem()
	{
		return sizeof(*this) + sizeof(neighbour_record) * neis_->capacity()
		    + nodepool_->mem();
	}

	// the expand function is responsible for generating the
	// successors of a search node. each generated successor
	// should be added to the list of neighbours via
	// ::add_neighbour
	virtual void
	expand(search_node*, search_problem_instance*)
	    = 0;

	// this function creates a search_node object for
	// represent a given start state described by @param pi.
	// the simplest concrete implementation is to call ::generate but
	// this assumes the identifier specified by @param pi is the same
	// as the one used internally by the domain model the expansion
	// policy is wrapping (e.g. a grid or a graph)
	//
	// @param pi: an problem describing a concrete start state
	// @return: a search_node object representing
	// the start state. if the start state is invalid the
	// function returns 0
	virtual search_node*
	generate_start_node(search_problem_instance* pi)
	    = 0;

	// this function creates a search_node object for
	// represent a given target state described by @param pi.
	// the simplest concrete implementation is to call ::generate but
	// this assumes the identifier specified by @param pi is the same
	// as the one used internally by the domain model the expansion
	// policy is wrapping (e.g. a grid or a graph)
	//
	// @param pi: an problem describing a concrete target state
	// @return: a search_node object representing
	// the target state. if the target state is invalid the
	// function returns 0
	virtual search_node*
	generate_target_node(search_problem_instance* pi)
	    = 0;

	virtual search_problem_instance
	get_problem_instance(problem_instance* pi)
	    = 0;

	virtual pack_id
	get_state(pad_id node_id)
	    = 0;

	virtual pad_id
	unget_state(pack_id node_id)
	    = 0;

	virtual void
	print_node(search_node* n, std::ostream& out)
	    = 0;

	// check if a given search node @param n corresponds to the
	// target. we do this here to decouple the internal
	// representation of states from the search algorithm which
	// only knows about search_node objects.
	bool
	is_target(search_node* n, search_problem_instance* pi)
	{
		return n->get_id() == pi->target_;
	}

	// get a search_node memory pointer associated with @param node_id.
	// (value is null if @param node_id is bigger than nodes_pool_size_)
	inline search_node*
	generate(pad_id node_id)
	{
		return nodepool_->generate(node_id);
	}

	// get the search_node memory pointer associated with @param node_id
	// value is null if this node has not been previously allocated
	// or if node_id is bigger than nodes_pool_size_
	search_node*
	get_ptr(pad_id node_id, uint32_t search_number)
	{
		search_node* tmp = nodepool_->get_ptr(node_id);
		if(tmp && tmp->get_search_number() == search_number) { return tmp; }
		return 0;
	}

protected:
	inline void
	add_neighbour(search_node* nei, double cost)
	{
		neis_->push_back(neighbour_record(nei, cost));
		// std::cout << " neis_.size() == " << neis_->size() << std::endl;
	}

	// return the index associated with the successor ::n()
	inline uint32_t
	get_current_successor_index()
	{
		return current_;
	}

private:
	struct neighbour_record
	{
		neighbour_record(search_node* node, double cost)
		{
			node_ = node;
			cost_ = cost;
		}
		search_node* node_;
		double cost_;
	};

	memory::node_pool* nodepool_;
	// std::vector<neighbour_record>* neis_;
	memory::arraylist<neighbour_record>* neis_;
	uint32_t current_;
	size_t nodes_pool_size_;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_EXPANSION_POLICY_H
