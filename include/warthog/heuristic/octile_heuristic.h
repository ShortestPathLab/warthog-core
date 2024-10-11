#ifndef WARTHOG_HEURISTIC_OCTILE_HEURISTIC_H
#define WARTHOG_HEURISTIC_OCTILE_HEURISTIC_H

// octile_heuristic.h
//
// Analogue of Manhattan Heuristic but for 8C grids (cf. 4C).
//
// @author: dharabor
// @created: 21/08/2012
//

#include "heuristic_value.h"
#include <warthog/constants.h>
#include <warthog/util/helpers.h>

namespace warthog::heuristic
{

class octile_heuristic
{
public:
	octile_heuristic(uint32_t mapwidth, uint32_t mapheight)
	    : mapwidth_(mapwidth), hscale_(1.0)
	{ }

	~octile_heuristic() { }

	double
	h(int32_t x, int32_t y, int32_t x2, int32_t y2)
	{
		int32_t dx = abs(x - x2);
		int32_t dy = abs(y - y2);
		if(dx < dy)
		{
			return (dx * warthog::DBL_ROOT_TWO + (dy - dx)) * hscale_;
		}
		return (dy * warthog::DBL_ROOT_TWO + (dx - dy)) * hscale_;
	}

	double
	h(sn_id_t id, sn_id_t id2)
	{
		int32_t x, x2;
		int32_t y, y2;
		warthog::util::index_to_xy((uint32_t)id, mapwidth_, x, y);
		warthog::util::index_to_xy((uint32_t)id2, mapwidth_, x2, y2);
		return this->h(x, y, x2, y2);
	}

	void
	h(heuristic_value* hv)
	{
		hv->lb_ = h(hv->from_, hv->to_);
	}

	void
	set_hscale(double hscale)
	{
		hscale_ = hscale;
	}

	double
	get_hscale()
	{
		return hscale_;
	}

	size_t
	mem()
	{
		return sizeof(this);
	}

private:
	unsigned int mapwidth_;
	double hscale_;
};

} // namespace warthog::heuristic

#endif // WARTHOG_HEURISTIC_OCTILE_HEURISTIC_H
