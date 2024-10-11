#ifndef WARTHOG_SEARCH_DUMMY_LISTENER_H
#define WARTHOG_SEARCH_DUMMY_LISTENER_H

// search/dummy_listener.h
//
// A search listener is a callback class that executes specialised
// code for partiular search events, such as when:
//  - a node is generated
//  - a node is expanded
//  - a node is relaxed
//
//  This class implements dummy listener with empty event handlers.
//
// @author: dharabor
// @created: 2020-03-09
//

#include "search_node.h"
#include <warthog/constants.h>
#include <warthog/forward.h>

namespace warthog::search
{

class dummy_listener
{
public:
	inline void
	generate_node(
	    search_node* parent, search_node* child, cost_t edge_cost,
	    uint32_t edge_id)
	{ }

	inline void
	expand_node(search_node* current)
	{ }

	inline void
	relax_node(search_node* current)
	{ }
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_DUMMY_LISTENER_H
