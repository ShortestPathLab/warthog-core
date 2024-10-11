#ifndef WARTHOG_SEARCH_UDS_TRAITS_H
#define WARTHOG_SEARCH_UDS_TRAITS_H

// search/uds_traits.h
//
// Traits that specify how a Uni-Directional Search should behave:
//   - to determine admissibility
//   - to determine termination
//   - to determine whether to reopen
//
// @author: dharabor
// @created: 2021-10-12
//

#include "search_metrics.h"
#include <warthog/util/log.h>

namespace warthog::search
{

////////////////////////////////////////////////////////////////////////////////
enum class admissibility_criteria
{
	any,
	w_admissible,
	eps_admissible
};

// test if the current solution is admissible
// our default approach always returns true solution; i.e.,
// a solution is admissible if it is feasible. other admissbility
// criteria (e.g., optimal, w-suboptimal. epislon-suboptimal etc)
// are handled via specialisation
//
// @param lb: node that establishes the current lower bound
// @param ub: node with the best solution so far

template<admissibility_criteria A>
inline bool
admissible(cost_t lb, cost_t ub, search_parameters* par)
{
	// default admissibility: any solution at all
	return ub != warthog::COST_MAX;
}

// w_admissibility:
// the current upperbound is not more than w * lowerbound, with w a user
// defined parameter (w=1 guarantees optimality).
template<>
inline bool
admissible<admissibility_criteria::w_admissible>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// TODO: precision issues can arise here. rounding would fix this
	// but we need to know a minimum cost-delta (round with half of that)
	assert(par->get_w_admissibility() >= 1.0);
	return ub <= (par->get_w_admissibility() * lb);
}

// eps_admissibility:
// the current upperbound is not more than eps(ilon) + lowerbound.
// Here eps is a user defined parameter (eps=0 guarantees optimality).
template<>
inline bool
admissible<admissibility_criteria::eps_admissible>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// TODO: precision issues can arise here. rounding would fix this
	// but we need to know a minimum cost-delta (round with half of that)
	assert(par->get_eps_admissibility() >= 0);
	return ub <= (par->get_eps_admissibility() + lb);
}

////////////////////////////////////////////////////////////////////////////////
enum class feasibility_criteria
{
	until_exhaustion,
	until_cutoff
};

// test if the search is still feasible; i.e., if a solution could still
// exist. our default approach is to suppose a solution still exists if
// there are more nodes to expand. other criteria (e.g., termination due
// to reaching some limit) are handled via specialisation
template<feasibility_criteria T>
inline bool
feasible(search_node* next, search_metrics* met, search_parameters* par)
{
	// default feasibility: still have unexpanded nodes
	return next;
}

template<>
inline bool
feasible<feasibility_criteria::until_cutoff>(
    search_node* next, search_metrics* met, search_parameters* par)
{
	if(next == nullptr) { return false; }

	if(next->get_f() > par->get_max_cost_cutoff())
	{
		info(
		    par->verbose_, "cost cutoff", next->get_f(), ">",
		    par->get_max_cost_cutoff());
		return false;
	}

	if(met->nodes_expanded_ >= par->get_max_expansions_cutoff())
	{
		info(
		    par->verbose_, "expansions cutoff", met->nodes_expanded_, ">",
		    par->get_max_expansions_cutoff());
		return false;
	}

	if(met->time_elapsed_nano_ > par->get_max_time_cutoff())
	{
		info(
		    par->verbose_, "time cutoff", met->time_elapsed_nano_, ">",
		    par->get_max_time_cutoff());
		return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////
enum class reopen_policy
{
	yes,
	no
};

// decide whether to renopen nodes already expanded (when their g-value
// can be improved). we handle the positive case via specialisation.
template<reopen_policy RO>
inline bool
reopen()
{
	return false;
}

template<>
inline bool
reopen<reopen_policy::yes>()
{
	return true;
}

} // namespace warthog::search

#endif // WARTHOG_SEARCH_UDS_TRAITS_H
