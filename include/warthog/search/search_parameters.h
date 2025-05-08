#ifndef WARTHOG_SEARCH_SEARCH_PARAMETERS_H
#define WARTHOG_SEARCH_SEARCH_PARAMETERS_H

// search/search_parameters.h
//
// A colleciton of parameters that can be used to configure heuristic search.
// These can take the form of various cutoff limits, that restrict the scope
// of a search, or they can be admissibility criteria that specify a desired
// level of quality for a returned solution.
//
// @author: dharabor, amaheo
// @created: 2021-10-13
//

#include <warthog/constants.h>

namespace warthog::search
{

struct search_parameters
{
public:
	search_parameters()
	{
		// defaults:
		// optimal solution cost, no cutoff limits
		cost_cutoff_       = DBL_MAX;
		exp_cutoff_        = UINT32_MAX;
		time_cutoff_ns_    = std::chrono::nanoseconds::max();
		w_admissibility_   = 1.0;
		eps_admissibility_ = 0.0;
		verbose_           = false;
	}

	// limits the maximum time which is available to find a solution
	// (in nanoseconds)
	void
	set_max_time_cutoff(std::chrono::nanoseconds nanos)
	{
		time_cutoff_ns_ = nanos;
	}

	std::chrono::nanoseconds
	get_max_time_cutoff()
	{
		return time_cutoff_ns_;
	}

	// convenience functions for setting larger time cutoffs

	void
	set_max_time_cutoff_s(double cutoff)
	{
		set_max_time_cutoff(
		    std::chrono::nanoseconds(static_cast<uint64_t>(cutoff * 1e9)));
	}

	// limits the maximum number of expansion operations
	void
	set_max_expansions_cutoff(uint32_t cutoff)
	{
		exp_cutoff_ = cutoff;
	}

	uint32_t
	get_max_expansions_cutoff()
	{
		return exp_cutoff_;
	}

	// limits the maximum solution cost
	void
	set_max_cost_cutoff(cost_t cutoff)
	{
		cost_cutoff_ = cutoff;
	}

	cost_t
	get_max_cost_cutoff()
	{
		return cost_cutoff_;
	}

	// tells the search an admissible solution is one whose
	// cost is not larger than optimal by more than a factor w >= 1
	void
	set_w_admissibility(double w)
	{
		assert(w >= 1);
		w_admissibility_ = w;
	}

	double
	get_w_admissibility()
	{
		return w_admissibility_;
	}

	// tells the search an admissible solution is one whose
	// cost is not larger than optimal by more than an additive
	// factor of epsilon >= 0
	void
	set_eps_admissibility(cost_t epsilon)
	{
		assert(epsilon >= 0);
		eps_admissibility_ = epsilon;
	};

	cost_t
	get_eps_admissibility()
	{
		return eps_admissibility_;
	}

	bool verbose_;

private:
	cost_t cost_cutoff_;
	uint32_t exp_cutoff_;
	std::chrono::nanoseconds time_cutoff_ns_;
	double w_admissibility_;
	cost_t eps_admissibility_ = 0;
};

} // namespace warthog::search

#endif // WARTHOG_SEARCH_SEARCH_PARAMETERS_H
