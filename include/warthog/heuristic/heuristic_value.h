#ifndef WARTHOG_HEURISTIC_HEURISTIC_VALUE_H
#define WARTHOG_HEURISTIC_HEURISTIC_VALUE_H

// heuristics/heuristic_value.h
//
// A container for passing data to and from heuristic functions.
//
// @author: dharabor
// @created: 2021-10-13
//

#include <vector>
#include <warthog/constants.h>

namespace warthog::heuristic
{

struct heuristic_value
{

	heuristic_value()
	{
		from_     = warthog::SN_ID_MAX;
		to_       = warthog::SN_ID_MAX;
		lb_       = warthog::COST_MAX;
		ub_       = warthog::COST_MAX;
		feasible_ = false;
		ub_path_  = 0;
	}

	heuristic_value(
	    pack_id from, pack_id to, std::vector<pack_id>* ub_path = 0)
	{
		this->operator()(sn_id_t{from}, sn_id_t{to}, ub_path);
	}
	heuristic_value(pad_id from, pad_id to, std::vector<pack_id>* ub_path = 0)
	{
		this->operator()(sn_id_t{from}, sn_id_t{to}, ub_path);
	}
	heuristic_value(
	    sn_id_t from, sn_id_t to, std::vector<pack_id>* ub_path = 0)
	{
		this->operator()(from, to, ub_path);
	}

	void
	operator()(sn_id_t from, sn_id_t to, std::vector<pack_id>* ub_path = 0)
	{
		from_     = from;
		to_       = to;
		lb_       = warthog::COST_MAX;
		ub_       = warthog::COST_MAX;
		feasible_ = false;
		ub_path_  = ub_path;
	}

	// lower and upperbound estimates
	cost_t lb_;
	cost_t ub_;

	// the pair of states the bounds refer to
	sn_id_t from_;
	sn_id_t to_;

	// are the bounds abstract estimates or
	// actually feasible plans?
	bool feasible_;

	// the container where the upperbound path
	// (if any) can be stored
	std::vector<pack_id>* ub_path_;
};

} // namespace warthog::heuristic

#endif // WARTHOG_HEURISTIC_HEURISTIC_VALUE_H
