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

class vl_gridmap_expansion_policy_base : public expansion_policy
{
public:
	vl_gridmap_expansion_policy_base(
	    domain::vl_gridmap* map, util::cost_table& costs);
	virtual ~vl_gridmap_expansion_policy_base();

	search_problem_instance
	get_problem_instance(problem_instance* pi) override;

	pack_id
	get_state(pad_id node_id) override;
	pad_id
	unget_state(pack_id node_id) override;

	/// get unpadded xy
	void
	get_xy(pack_id node_id, int32_t& x, int32_t& y);
	/// get unpadded xy
	void
	get_xy(pad_id node_id, int32_t& x, int32_t& y);

	/// unpadded xy to pack
	pack_id get_pack(int32_t x, int32_t y);
	/// unpadded xy to pad
	pad_id get_pad(int32_t x, int32_t y);

	domain::vl_gridmap* get_map() const noexcept { return map_; }
	const util::cost_table& get_costs() const noexcept { return costs_; }

	void
	print_node(search_node* n, std::ostream& out) override;

	size_t
	mem() override;

protected:
	domain::vl_gridmap* map_;
	util::cost_table& costs_;
};

class vl_gridmap_expansion_policy : public vl_gridmap_expansion_policy_base
{
public:
	using vl_gridmap_expansion_policy_base::vl_gridmap_expansion_policy_base;

	void
	expand(search_node*, search_problem_instance*) override;

	search_node*
	generate_start_node(search_problem_instance* pi) override;

	search_node*
	generate_target_node(search_problem_instance* pi) override;

	size_t
	mem() override;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_VL_GRIDMAP_EXPANSION_POLICY_H
