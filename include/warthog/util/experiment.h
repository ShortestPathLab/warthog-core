#ifndef WARTHOG_UTIL_EXPERIMENT_H
#define WARTHOG_UTIL_EXPERIMENT_H

// experiment.h
//
// An object for holding experiments read from Nathan Sturtevant's
// .scenario files.
// Based on an implementation from HOG by Renee Jansen.
//
// NB: This implementation makes use of an additional attribute,
// ::precision_, which can be used to indicate the accuracy with which
// ::distance_ should be interpreted. The hardcoded default is 4.
//
// NB2: The attributes ::mapwidth_ and ::mapheight_ refer to the x/y dimensions
// that ::map should be scaled to. The individial node
// coordinates (::startx_, ::starty_ etc.) are taken with respect to the
// dimensions of the scaled map.
//
// @author: dharabor
// @created: 21/08/2012
//

#include <warthog/search/problem_instance.h>

#include <iostream>
#include <string>

namespace warthog::util
{

class experiment
{
public:
	experiment(
	    uint32_t sx, uint32_t sy, uint32_t gx, uint32_t gy, uint32_t mapwidth,
	    uint32_t mapheight, double d, std::string m)
	    : startx_(sx), starty_(sy), goalx_(gx), goaly_(gy),
	      mapwidth_(mapwidth), mapheight_(mapheight), distance_(d), map_(m),
	      precision_(4)
	{ }

	// no copy
	experiment(const experiment& other) = delete;
	experiment&
	operator=(const experiment& other)
	    = delete;

	~experiment() { }

	uint32_t
	startx() const noexcept
	{
		return startx_;
	}

	uint32_t
	starty() const noexcept
	{
		return starty_;
	}

	uint32_t
	goalx() const noexcept
	{
		return goalx_;
	}

	uint32_t
	goaly() const noexcept
	{
		return goaly_;
	}

	double
	distance() const noexcept
	{
		return distance_;
	}

	const std::string&
	map() const noexcept
	{
		return map_;
	}

	uint32_t
	mapwidth() const noexcept
	{
		return mapwidth_;
	}

	uint32_t
	mapheight() const noexcept
	{
		return mapheight_;
	}

	int32_t
	precision() const noexcept
	{
		return precision_;
	}

	void
	set_precision(int32_t prec) noexcept
	{
		precision_ = prec;
	}

	void
	print(std::ostream& out);

	search::problem_instance
	get_instance() const noexcept
	{
		return search::problem_instance(
		    pack_id{starty_ * mapwidth_ + startx_},
		    pack_id{goaly_ * mapwidth_ + goalx_});
	}

private:
	uint32_t startx_, starty_, goalx_, goaly_;
	uint32_t mapwidth_, mapheight_;
	double distance_;
	std::string map_;
	int32_t precision_;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_EXPERIMENT_H
