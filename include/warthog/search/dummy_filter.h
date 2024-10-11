#ifndef WARTHOG_SEARCH_DUMMY_FILTER_H
#define WARTHOG_SEARCH_DUMMY_FILTER_H

// search/dummy_filter.h
//
// A node filter that doesn't filter anything.
//
// @author: dharabor
// @created: 2016-07-19
//

#include <cstdint>

namespace warthog::search
{

class dummy_filter
{
public:
	dummy_filter() { }
	~dummy_filter() { }

	inline bool
	filter(uint32_t node_id, uint32_t edge_idx)
	{
		return false;
	}

	inline void
	set_target(uint32_t target_id)
	{ }
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_DUMMY_FILTER_H
