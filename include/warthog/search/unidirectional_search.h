#ifndef WARTHOG_SEARCH_UNIDIRECTIONAL_SEARCH_H
#define WARTHOG_SEARCH_UNIDIRECTIONAL_SEARCH_H

// search/unidirectional_search.h
//
// Unidirectional search whose algorithmic parameters and properties
// can be specified via templates.
//
// @author: dharabor, amaheo
// @created: 2021-10-13
//

#include "problem_instance.h"
#include "search.h"
#include "search_parameters.h"
#include "solution.h"
#include "uds_traits.h"
#include <warthog/io/listener.h>
#include <warthog/constants.h>
#include <warthog/heuristic/heuristic_value.h>
#include <warthog/memory/cpool.h>
#include <warthog/util/log.h>
#include <warthog/util/pqueue.h>
#include <warthog/util/timer.h>
#include <warthog/util/vec_io.h>

#include <functional>
#include <iostream>
#include <memory>
#include <vector>

namespace warthog::search
{

// H is a heuristic function
// E is an expansion policy
// Q is the open list
// L is a "listener" which is used for callbacks
// T is the search traits that specify the admissibility criteria
// required for a solution to be returned, and feasibility criteria
// used determine if a search should continue or terminate.
// (default: search for any solution, until OPEN is exhausted)
template<
    typename H, typename E, typename Q = util::pqueue_min, typename L = std::tuple<>,
    admissibility_criteria AC = admissibility_criteria::any,
    feasibility_criteria FC   = feasibility_criteria::until_exhaustion,
    reopen_policy RP          = reopen_policy::no>
class unidirectional_search
{
public:
	unidirectional_search(
	    H* heuristic, E* expander, Q* queue, L listeners = L{})
	    : heuristic_(heuristic), expander_(expander), open_(queue), listeners_(listeners)
	{ }

	~unidirectional_search() { }

	void
	get_pathcost(problem_instance* pi, search_parameters* par, solution* sol)
	{
		search(pi, par, sol);
	}

	void
	get_path(problem_instance* pi, search_parameters* par, solution* sol)
	{
		search_problem_instance spi = expander_->get_problem_instance(pi);
		get_path(&spi, par, sol);
	}
	void
	get_path(
	    search_problem_instance* spi, search_parameters* par, solution* sol)
	{
		// if successful the search returns an incumbent node. this can be
		// the target node or it can be another node from which the
		// heuristic knows a concrete path to the target.
		search(spi, par, sol);
		if(!sol->s_node_) { return; }

		// follow backpointers to extract the path, from start to incumbent
		search_node* current = sol->s_node_;
		while(current)
		{
			sol->path_.push_back(expander_->get_state(current->get_id()));
			if(current->get_parent() == pad_id::max()) break;
			current = expander_->generate(current->get_parent());
		}
		assert(sol->path_.back() == expander_->get_state(spi->start_));
		std::reverse(sol->path_.begin(), sol->path_.end());

		// extract the rest of the path, from incumbent to target
		if(sol->s_node_->get_id() != spi->target_)
		{
			heuristic::heuristic_value hv(
			    sol->s_node_->get_id(), spi->target_, &sol->path_);
			heuristic_->h(&hv);
		}

		DO_ON_DEBUG_IF(spi->verbose_)
		{
			for(auto& node_id : sol->path_)
			{
				int32_t x, y;
				expander_->get_xy(node_id, x, y);
				std::cerr << "final path: (" << x << ", " << y << ")...";
				search_node* n
				    = expander_->generate(expander_->unget_state(node_id));
				assert(n->get_search_number() == spi->instance_id_);
				n->print(std::cerr);
				std::cerr << std::endl;
			}
		}
	}

	L&
	get_listeners() noexcept
	{
		return listeners_;
	}

	E*
	get_expander()
	{
		return expander_;
	}

	H*
	get_heuristic()
	{
		return heuristic_;
	}

	inline size_t
	mem()
	{
		size_t bytes =
		    // memory for the priority quete
		    open_->mem() +
		    // gridmap size and other stuff needed to expand nodes
		    expander_->mem() +
		    // heuristic uses some memory too
		    heuristic_->mem() +
		    // misc
		    sizeof(*this);
		return bytes;
	}

private:
	// search parameters
	H* heuristic_;
	E* expander_;
	Q* open_;
	[[no_unique_address]] L listeners_;

	// no copy ctor
	unidirectional_search(const unidirectional_search& other) { }
	unidirectional_search&
	operator=(const unidirectional_search& other)
	{
		return *this;
	}

	/**
	 * Initialise a new 'search_node' for the ongoing search given the parent
	 * node (@param current).
	 */
	void
	initialise_node_(
	    search_node* n, pad_id parent_id, cost_t gval,
	    search_problem_instance* pi, search_parameters* par, solution* sol)
	{
		heuristic::heuristic_value hv(n->get_id(), pi->target_);
		heuristic_->h(&hv);

		// NB: unlikely, but node cost  overflow could occur
		assert((warthog::COST_MAX - hv.lb_) > gval);
		assert(
		    hv.ub_ == warthog::COST_MAX
		    || ((warthog::COST_MAX - hv.ub_) > gval));

		n->init(
		    pi->instance_id_, parent_id, gval,
		    gval + (hv.lb_ * par->get_w_admissibility()),
		    (gval * hv.feasible_) + hv.ub_);

		// update the incumbent solution
		bool is_target = n->get_id() == pi->target_;
		if((is_target || hv.feasible_) && gval < sol->sum_of_edge_costs_)
		{
			sol->s_node_            = n;
			sol->sum_of_edge_costs_ = gval;
		}
	}

	void
	update_ub(search_node* n, solution* sol, search_problem_instance* pi)
	{
		if(n->get_ub() < sol->met_.ub_)
		{
			sol->met_.ub_ = n->get_ub();
			debug(
			    pi->verbose_, "NEW UB:", "Incumbent Cost",
			    sol->sum_of_edge_costs_);
		}
	}

	void
	search(search_problem_instance* pi, search_parameters* par, solution* sol)
	{
		util::timer mytimer;
		mytimer.start();
		open_->clear();

		io::listener_begin_search(listeners_, static_cast<int>(pi->instance_id_), *pi);

		// initialise the start node and push to OPEN
		{
			if(pi->start_ == pad_id::max()) { return; }

			search_node* start = expander_->generate_start_node(pi);
			if(!start) { return; }
			// search_node* target = expander_->generate_target_node(pi);
			// pi.target_ = target.id_;

			initialise_node_(start, pad_id::max(), 0, pi, par, sol);
			open_->push(start);
			io::listener_generate_node(listeners_, nullptr, *start, 0, UINT32_MAX);
			user(pi->verbose_, pi);
			trace(pi->verbose_, "Start node:", *start);
			update_ub(start, sol, pi);
		}

		// keep expanding until it is no longer feasible to do so;
		// e.g., we exceeded a cutoff or prove that no solution exists
		while(feasible<FC>(open_->peek(), &sol->met_, par))
		{
			// check if the incumbent solution is admissible
			if(admissible<AC>(
			       open_->peek()->get_f(), sol->sum_of_edge_costs_, par))
			{
				break;
			}

			// incumbent is not not admissible. expand the most
			// promising node from the OPEN list:
			search_node* current = open_->pop();
			expander_->expand(current, pi);
			current->set_expanded(true); // NB: set before generating succ
			sol->met_.nodes_expanded_++;
			sol->met_.lb_ = current->get_f();
			io::listener_expand_node(listeners_, *current);
			trace(pi->verbose_, "Expanding:", *current);

			// Generate successors of the current node
			search_node* n   = nullptr;
			cost_t cost_to_n = warthog::COST_MAX;
			for(uint32_t i = 0; i < expander_->get_num_successors(); i++)
			{
				expander_->get_successor(i, n, cost_to_n);
				sol->met_.nodes_generated_++;
				cost_t gval = current->get_g() + cost_to_n;

				// Generate new search nodes, provided they're not
				// dominated by the current upperbound
				if(n->get_search_number() != current->get_search_number())
				{
					initialise_node_(n, current->get_id(), gval, pi, par, sol);
					if(n->get_f() <= sol->sum_of_edge_costs_)
					{
						open_->push(n);
						trace(pi->verbose_, "Generate:", *n);
						update_ub(current, sol, pi);
						io::listener_generate_node(listeners_, current, *n, gval, i);
						continue;
					}
				}

				// relax and reopen, but only if the new lowerbound
				// for the node is less than the current upperbound
				if(gval < n->get_g())
				{
					if((gval + n->get_f() - n->get_g())
					   < sol->sum_of_edge_costs_)
					{
						n->relax(gval, current->get_id());
						io::listener_relax_node(listeners_, *n);

						if(open_->contains(n))
						{
							open_->decrease_key(n);
							trace(pi->verbose_, "Updating;", *n);
							update_ub(current, sol, pi);
							continue;
						}

						if(reopen<RP>())
						{
							open_->push(n);
							trace(pi->verbose_, "Reopen;", *n);
							update_ub(current, sol, pi);
							sol->met_.nodes_reopen_++;
							continue;
						}
					}
				}
				trace(pi->verbose_, "Dominated;", *n);
			}
			if constexpr(FC == feasibility_criteria::until_cutoff)
			{
				// patched until AC FC RP reworked
				sol->met_.time_elapsed_nano_ = mytimer.elapsed_time_nano();
			}
			io::listener_close_node(listeners_, *current);
		}

		sol->met_.time_elapsed_nano_ = mytimer.elapsed_time_nano();
		sol->met_.nodes_surplus_     = open_->size();
		sol->met_.heap_ops_          = open_->get_heap_ops();

		DO_ON_DEBUG_IF(pi->verbose_)
		{
			if(sol->sum_of_edge_costs_ == warthog::COST_MAX)
			{
				warning(pi->verbose_, "Search failed; no solution exists.");
			}
			else { user(pi->verbose_, "Solution found", *sol->s_node_); }
		}
	}
};

template<
    typename H, typename E, typename Q = util::pqueue_min, typename L = std::tuple<>>
unidirectional_search(
    H* heuristic, E* expander, Q* queue,
    L listeners = L{}) -> unidirectional_search<H, E, Q, L>;

} // namespace warthog::search

#endif // WARTHOG_SEARCH_UNIDIRECTIONAL_SEARCH_H
