#ifndef WARTHOG_SCENARIO_EXPERIMENT_H
#define WARTHOG_SCENARIO_EXPERIMENT_H

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
#include <string_view>

namespace warthog::scenario
{

struct experiment
{
	experiment(
	    uint32_t sx, uint32_t sy, uint32_t gx, uint32_t gy, uint32_t mapwidth,
	    uint32_t mapheight, std::optional<double> d, std::string_view m)
	    : startx_(sx), starty_(sy), goalx_(gx), goaly_(gy),
	      mapwidth_(mapwidth), mapheight_(mapheight), distance_(d), map_(m)
	{ }
	experiment(
	    double sx, double sy, double gx, double gy, uint32_t mapwidth,
	    uint32_t mapheight, std::optional<double> d, std::string_view m)
	    : startx_(sx), starty_(sy), goalx_(gx), goaly_(gy),
	      mapwidth_(mapwidth), mapheight_(mapheight), distance_(d), map_(m)
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
		return static_cast<uint32_t>(startx_);
	}
	double
	startx_f() const noexcept
	{
		return startx_;
	}

	uint32_t
	starty() const noexcept
	{
		return static_cast<uint32_t>(starty_);
	}
	double
	starty_f() const noexcept
	{
		return starty_;
	}

	uint32_t
	goalx() const noexcept
	{
		return static_cast<uint32_t>(goalx_);
	}
	double
	goalx_f() const noexcept
	{
		return goalx_;
	}

	uint32_t
	goaly() const noexcept
	{
		return static_cast<uint32_t>(goaly_);
	}
	double
	goaly_f() const noexcept
	{
		return goaly_;
	}

	std::optional<double>
	distance() const noexcept
	{
		return distance_;
	}

	std::string_view
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

	void
	print(std::ostream& out);

	search::problem_instance
	get_instance() const noexcept
	{
		return search::problem_instance(
		    pack_id{starty() * mapwidth_ + startx()},
		    pack_id{goaly() * mapwidth_ + goalx()});
	}

	double startx_, starty_, goalx_, goaly_;
	uint32_t mapwidth_, mapheight_;
	std::optional<double> distance_; ///< -1 = no solution, non-init = unknown solution
	std::string_view map_;
};

inline void
experiment::print(std::ostream& out)
{
	out << this->map() << "\t";
	out << this->mapwidth() << "\t";
	out << this->mapheight() << "\t";
	out << this->startx() << "\t";
	out << this->starty() << "\t";
	out << this->goalx() << "\t";
	out << this->goaly() << "\t";
	if (this->distance())
		out << std::setprecision(10) << *this->distance();
	else
		out << '-';
}

} // namespace warthog::scenario

#endif // WARTHOG_SCENARIO_EXPERIMENT_H
