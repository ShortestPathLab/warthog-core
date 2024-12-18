#ifndef WARTHOG_SEARCH_GRIDMAP_EXPANSION_POLICY_H
#define WARTHOG_SEARCH_GRIDMAP_EXPANSION_POLICY_H

// gridmap_expansion_policy.h
//
// An ExpansionPolicy for square uniform-cost grids.
// Supports octile movement and manhattan movement.
//
// In the case where diagonal moves are allowed
// corner-cutting is forbidden.
// i.e.
//
// ab
// cd
//
// the move c -> b is only permitted if both 'a' and 'd'
// are traversable.
//
// @author: dharabor
// @created: 28/10/2010
//

#include "expansion_policy.h"
#include "problem_instance.h"
#include "search_node.h"
#include <warthog/domain/gridmap.h>

#include <memory>

namespace warthog::search
{

class gridmap_expansion_policy_base : public expansion_policy
{
public:
	gridmap_expansion_policy_base(domain::gridmap* map);

	search_problem_instance
	get_problem_instance(problem_instance* pi) override;

	pack_id
	get_state(pad_id node_id) override;
	pad_id
	unget_state(pack_id node_id) override;

	/// get unpadded xy
	void
	get_xy(pack_id node_id, int32_t& x, int32_t& y);
	/// get padded xy
	void
	get_xy(pad_id node_id, int32_t& x, int32_t& y);

	/// get unpadded xy
	pack_id get_pack(int32_t x, int32_t y);
	/// get padded xy
	pad_id get_pad(int32_t x, int32_t y);

	domain::gridmap* get_grid() const noexcept { return map_; }

	void
	print_node(search_node* n, std::ostream& out) override;

	size_t
	mem() override;

protected:
	domain::gridmap* map_;
};

class gridmap_expansion_policy : public gridmap_expansion_policy_base
{
public:
	gridmap_expansion_policy(domain::gridmap* map, bool manhattan = false);

	void
	expand(search_node*, search_problem_instance*) override;

	search_node*
	generate_start_node(search_problem_instance* pi) override;

	search_node*
	generate_target_node(search_problem_instance* pi) override;

	size_t
	mem() override;

protected:
	bool manhattan_;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_GRIDMAP_EXPANSION_POLICY_H
