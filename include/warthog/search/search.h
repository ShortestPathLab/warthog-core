#ifndef WARTHOG_SEARCH_SEARCH_H
#define WARTHOG_SEARCH_SEARCH_H

// search.h
//
// A base class for functionality common to all
// search algorithms.
//
// @author: dharabor
// @created: 2016-02-24
//

#include "problem_instance.h"
#include "solution.h"

#include <stdint.h>
#include <stdlib.h>

namespace warthog::search
{

class search
{
public:
	search() { }
	virtual ~search() { }

	virtual void
	get_path(problem_instance&, solution&)
	    = 0;

	virtual void
	get_pathcost(problem_instance&, solution&)
	    = 0;

	virtual size_t
	mem()
	    = 0;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_SEARCH_H
