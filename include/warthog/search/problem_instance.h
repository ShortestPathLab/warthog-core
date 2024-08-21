#ifndef WARTHOG_SEARCH_PROBLEM_INSTANCE_H
#define WARTHOG_SEARCH_PROBLEM_INSTANCE_H

#include "search_node.h"

namespace warthog::search
{

static uint32_t instance_counter_ = UINT32_MAX;

template<typename STATE>
class problem_instance_base
{
public:
	problem_instance_base(STATE start, STATE target, bool verbose = 0)
	    : start_(start), target_(target), instance_id_(++instance_counter_),
	      verbose_(verbose), extra_params_(0)

	{ }

	problem_instance_base(const problem_instance_base<STATE>& other)
	{
		this->start_ = other.start_;
		this->target_ = other.target_;
		this->instance_id_ = instance_counter_++;
		this->verbose_ = other.verbose_;
		this->extra_params_ = other.extra_params_;
	}

	~problem_instance_base() { }

	void
	reset()
	{
		instance_id_ = instance_counter_++;
	}

	problem_instance_base<STATE>&
	operator=(const problem_instance_base<STATE>& other)
	{
		this->start_ = other.start_;
		this->target_ = other.target_;
		this->instance_id_ = instance_counter_++;
		this->verbose_ = other.verbose_;
		this->extra_params_ = other.extra_params_;
		return *this;
	}

	void
	print(std::ostream& out)
	{
		out << "problem instance; start = " << start_ << " " << " target "
		    << target_ << " " << " search_id " << instance_id_;
	}

	STATE start_;
	STATE target_;
	uint32_t instance_id_;
	bool verbose_;

	// stuff we might want to pass in
	void* extra_params_;
};

typedef problem_instance_base<sn_id_t> problem_instance;

} // namespace warthog::search

std::ostream&
operator<<(std::ostream& str, warthog::search::problem_instance& pi);

#endif // WARTHOG_SEARCH_PROBLEM_INSTANCE_H
