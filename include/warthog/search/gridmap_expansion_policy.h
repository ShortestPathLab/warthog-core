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

class gridmap_expansion_policy : public expansion_policy
{
public:
	gridmap_expansion_policy(domain::gridmap* map, bool manhattan = false);
	virtual ~gridmap_expansion_policy() { }

	virtual void
	expand(search_node*, problem_instance*);

	uint32_t
	get_state(sn_id_t node_id);

	void
	get_xy(sn_id_t node_id, int32_t& x, int32_t& y);

	void
	print_node(search_node* n, std::ostream& out);

	virtual search_node*
	generate_start_node(problem_instance* pi);

	virtual search_node*
	generate_target_node(problem_instance* pi);

	virtual size_t
	mem();

private:
	domain::gridmap* map_;
	bool manhattan_;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_GRIDMAP_EXPANSION_POLICY_H
