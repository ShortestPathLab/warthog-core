#ifndef WARTHOG_SEARCH_VL_GRIDMAP_EXPANSION_POLICY_H
#define WARTHOG_SEARCH_VL_GRIDMAP_EXPANSION_POLICY_H

// search/vl_gridmap_expansion_policy.h
//
// An expansion policy for square gridmaps with
// vertex costs. There are eight possible move
// actions: 4 cardinal and 4 diagonal with
// costs 1 and sqrt(2) respectively.
//
// The edge costs are weighted by the average of
// the vertices they connect.
//
// Example:
//
// ab
// cd
//
// In the case of a cardial move from cell a to b,
// we compute the dge weight by computing the average
// of the the labels found at vertex a and vertex b.
// Diagonal moves are similar but we take the
// average of four cells
//
//
// @author: dharabor
// @created: 2014-09-17
// @updated: 2018-11-09
//

#include "expansion_policy.h"
#include "search_node.h"
#include <warthog/domain/labelled_gridmap.h>
#include <warthog/util/cost_table.h>

#include <memory>

namespace warthog::search
{

class vl_gridmap_expansion_policy : public expansion_policy
{
public:
	vl_gridmap_expansion_policy(
	    domain::vl_gridmap* map, util::cost_table& costs);
	virtual ~vl_gridmap_expansion_policy();

	virtual void
	expand(search_node*, problem_instance*);

	virtual size_t
	mem()
	{
		return expansion_policy::mem() + sizeof(*this) + map_->mem();
	}

	uint32_t
	get_state(sn_id_t node_id);

	void
	print_node(search_node* n, std::ostream& out);

	virtual search_node*
	generate_start_node(problem_instance* pi);

	virtual search_node*
	generate_target_node(problem_instance* pi);

private:
	domain::vl_gridmap* map_;
	util::cost_table& costs_;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_VL_GRIDMAP_EXPANSION_POLICY_H
