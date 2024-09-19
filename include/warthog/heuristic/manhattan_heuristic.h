#ifndef WARTHOG_HEURISTIC_MANHATTAN_HEURISTIC_H
#define WARTHOG_HEURISTIC_MANHATTAN_HEURISTIC_H

// manhattan_heuristic.h
//
// @author: dharabor
// @created: 21/08/2012
//

#include "heuristic_value.h"
#include <warthog/constants.h>
#include <warthog/util/helpers.h>

namespace warthog::heuristic
{

class manhattan_heuristic
{
public:
	manhattan_heuristic(uint32_t mapwidth, uint32_t mapheight)
	    : mapwidth_(mapwidth)
	{ }

	~manhattan_heuristic() { }

	double
	h(int32_t x, int32_t y, int32_t x2, int32_t y2)
	{
		// NB: precision loss when double is an integer
		return (abs(x - x2) + abs(y - y2));
	}

	double
	h(sn_id_t id, sn_id_t id2)
	{
		int32_t x, x2;
		int32_t y, y2;
		util::index_to_xy((uint32_t)id, mapwidth_, x, y);
		util::index_to_xy((uint32_t)id2, mapwidth_, x2, y2);
		return this->h(x, y, x2, y2);
	}

	void
	h(heuristic_value* hv)
	{
		hv->lb_ = h(hv->from_, hv->to_);
	}

	size_t
	mem()
	{
		return sizeof(this);
	}

private:
	uint32_t mapwidth_;
};

} // namespace warthog::heuristic

#endif // WARTHOG_HEURISTIC_MANHATTAN_HEURISTIC_H
