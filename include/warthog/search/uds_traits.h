#ifndef WARTHOG_SEARCH_UDS_TRAITS_H
#define WARTHOG_SEARCH_UDS_TRAITS_H

/// @file uds_traits.h
///
/// Traits that specify how a Uni-Directional Search should behave:
///   - to determine admissibility
///   - to determine termination
///   - to determine whether to reopen
///
/// Traits are applied through a trait class, see uds_traits for an example
/// of a complete (and user definable) class of all traits.
/// Every trait has a default value, and thus are optional to include, i.e.
/// an empty struct is the same as uds_default_traits.
///
/// @author: dharabor & Ryan Hechenberger
/// @created: 2021-10-12

#include "search_metrics.h"
#include <warthog/io/log.h>
#include <warthog/util/template.h>

namespace warthog::search
{

////////////////////////////////////////////////////////////////////////////////
enum class admissibility_criteria
{
	any,
	optimal,
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
admissible(cost_t lb, cost_t ub, search_parameters* par);

/// any:
/// the any admissible approach always returns true solution; i.e.,
/// a solution is admissible if it is feasible.
template<>
inline bool
admissible<admissibility_criteria::any>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// default admissibility: any solution at all
	return ub != warthog::COST_MAX;
}

/// optimal:
/// the optimal admissible approach returns when the lower-bound has
/// reached or exceeded the upper bound i.e., no better solution
/// exists in the queue.
template<>
inline bool
admissible<admissibility_criteria::optimal>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// default admissibility: any solution at all
	return lb >= ub;
}

/// w_admissibility:
/// the current upperbound is not more than w * lowerbound, with w a user
/// defined parameter (w=1 guarantees optimality).
template<>
inline bool
admissible<admissibility_criteria::w_admissible>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// TODO: precision issues can arise here. rounding would fix this
	// but we need to know a minimum cost-delta (round with half of that)
	assert(par->get_w_admissibility() >= 1.0);
	return (par->get_w_admissibility() * lb) >= ub;
}

/// eps_admissibility:
/// the current upperbound is not more than eps(ilon) + lowerbound.
/// Here eps is a user defined parameter (eps=0 guarantees optimality).
template<>
inline bool
admissible<admissibility_criteria::eps_admissible>(
    cost_t lb, cost_t ub, search_parameters* par)
{
	// TODO: precision issues can arise here. rounding would fix this
	// but we need to know a minimum cost-delta (round with half of that)
	assert(par->get_eps_admissibility() >= 0);
	return (par->get_eps_admissibility() + lb) >= ub;
}

////////////////////////////////////////////////////////////////////////////////
enum class feasibility_criteria
{
	until_exhaustion,
	until_cutoff
};

/// test if the search is still feasible; i.e., if a solution could still
/// exist.
template<feasibility_criteria T>
inline bool
feasible(search_node* next, search_metrics* met, search_parameters* par);

/// test if the search is still feasible; i.e., if a solution could still
/// exist. our default approach is to suppose a solution still exists if
/// there are more nodes to expand. other criteria (e.g., termination due
/// to reaching some limit) are handled via specialisation
template<>
inline bool
feasible<feasibility_criteria::until_exhaustion>(
    search_node* next, search_metrics* met, search_parameters* par)
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
		WARTHOG_GINFO_FMT_IF(
		    par->verbose_, "cost cutoff {} > {}", next->get_f(),
		    par->get_max_cost_cutoff());
		return false;
	}

	if(met->nodes_expanded_ >= par->get_max_expansions_cutoff())
	{
		WARTHOG_GINFO_FMT_IF(
		    par->verbose_, "expansions cutoff {} > {}", met->nodes_expanded_,
		    par->get_max_expansions_cutoff());
		return false;
	}

	if(met->time_elapsed_nano_ > par->get_max_time_cutoff())
	{
		WARTHOG_GINFO_FMT_IF(
		    par->verbose_, "time cutoff {} > {}", met->time_elapsed_nano_,
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
template<reopen_policy R>
inline bool
reopen();

template<>
inline bool
reopen<reopen_policy::no>()
{
	return false;
}

template<>
inline bool
reopen<reopen_policy::yes>()
{
	return true;
}

/// The default parameters given in the class for uds traits.
/// Is equivalent to an empty struct in behaviour, as
/// admissibility_criteria::any, feasibility_criteria::until_exhaustion
/// and reopen_policy::no are the default values in uds.
struct uds_default_traits
{
	using node = search_node;
	using observer = std::tuple<>;
	static constexpr admissibility_criteria ac = admissibility_criteria::any;
	static constexpr feasibility_criteria fc
	    = feasibility_criteria::until_exhaustion;
	static constexpr reopen_policy rp = reopen_policy::no;
};

/// Modify search behaviour of unidirectional_search.
/// To be passed as template parameters with compile-time values
/// and types.
/// These values are optional and a user-provided struct only require
/// parameter for values that differ to the default.
template<
    typename N = uds_default_traits::node,
	typename L = uds_default_traits::observer,
    admissibility_criteria AC = uds_default_traits::ac,
    feasibility_criteria FC   = uds_default_traits::fc,
    reopen_policy RP          = uds_default_traits::rp>
struct uds_traits
{
	using node                                 = N;
	using observer                             = L;
	static constexpr admissibility_criteria ac = AC;
	static constexpr feasibility_criteria fc   = FC;
	static constexpr reopen_policy rp          = RP;
};

namespace details
{

template<typename Traits>
struct uds_trait_node
{
	using type = uds_default_traits::node;
};
template<typename Traits>
    requires requires { typename Traits::node; }
struct uds_trait_node<Traits>
{
	using type = typename Traits::node;
};

template<typename Traits>
struct uds_trait_observer
{
	using type = uds_default_traits::observer;
};
template<typename Traits>
    requires requires { typename Traits::observer; }
struct uds_trait_observer<Traits>
{
	using type = typename Traits::observer;
};

} // namespace details

template<typename Traits>
using uds_trait_node = typename details::uds_trait_node<Traits>::type;

template<typename Traits>
using uds_trait_observer = typename details::uds_trait_observer<Traits>::type;

template<typename Traits>
inline consteval admissibility_criteria
uds_trait_ac() noexcept
{
	if constexpr(requires {
		             {
			             Traits::ac
		             } -> util::same_as_rmref<admissibility_criteria>;
	             })
	{
		return Traits::ac;
	}
	else { return uds_default_traits::ac; }
}

template<typename Traits>
inline consteval feasibility_criteria
uds_trait_fc() noexcept
{
	if constexpr(requires {
		             {
			             Traits::fc
		             } -> util::same_as_rmref<feasibility_criteria>;
	             })
	{
		return Traits::fc;
	}
	else { return uds_default_traits::fc; }
}

template<typename Traits>
inline consteval reopen_policy
uds_trait_rp() noexcept
{
	if constexpr(requires {
		             { Traits::rp } -> util::same_as_rmref<reopen_policy>;
	             })
	{
		return Traits::rp;
	}
	else { return uds_default_traits::rp; }
}

} // namespace warthog::search

#endif // WARTHOG_SEARCH_UDS_TRAITS_H
