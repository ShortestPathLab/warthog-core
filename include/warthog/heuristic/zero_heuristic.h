#ifndef WARTHOG_HEURISTIC_ZERO_HEURISTIC_H
#define WARTHOG_HEURISTIC_ZERO_HEURISTIC_H

// zero_heuristic.h
//
// @author: dharabor
// @created: 2014-10-22
//

#include <warthog/constants.h>
#include <warthog/util/helpers.h>
#include "heuristic_value.h"

#include <cstdlib>

namespace warthog::heuristic
{

class zero_heuristic
{
public:
	zero_heuristic() { }
	~zero_heuristic() { }

	double
	h(unsigned int x, unsigned int y, unsigned int x2, unsigned int y2)
	{
		return 0;
	}

	double
	h(sn_id_t id, sn_id_t id2)
	{
		return 0;
	}

	void
	h(heuristic_value* hv)
	{
		hv->lb_ = hv->ub_ = 0;
	}

	size_t
	mem()
	{
		return sizeof(this);
	}
};

} // namespace warthog::heuristic

#endif // WARTHOG_HEURISTIC_ZERO_HEURISTIC_H
