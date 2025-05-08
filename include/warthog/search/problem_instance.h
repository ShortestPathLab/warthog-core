#ifndef WARTHOG_SEARCH_PROBLEM_INSTANCE_H
#define WARTHOG_SEARCH_PROBLEM_INSTANCE_H

#include "search_node.h"

namespace warthog::search
{

static uint32_t instance_counter_ = UINT32_MAX;

template<Identity STATE>
class problem_instance_base
{
public:
	constexpr problem_instance_base(
	    STATE start, STATE target, bool verbose = false) noexcept
	    : start_(start), target_(target), instance_id_(++instance_counter_),
	      verbose_(verbose), extra_params_(nullptr)
	{ }
	constexpr problem_instance_base(
	    STATE start, STATE target, uint32_t instance_id, bool verbose,
	    void* extra_params = nullptr) noexcept
	    : start_(start), target_(target), instance_id_(++instance_counter_),
	      verbose_(verbose), extra_params_(nullptr)
	{ }

	constexpr problem_instance_base(const problem_instance_base<STATE>& other)
	{
		this->start_        = other.start_;
		this->target_       = other.target_;
		this->instance_id_  = instance_counter_++;
		this->verbose_      = other.verbose_;
		this->extra_params_ = other.extra_params_;
	}

	// ~problem_instance_base() { }

	constexpr void
	reset()
	{
		instance_id_ = instance_counter_++;
	}

	constexpr problem_instance_base<STATE>&
	operator=(const problem_instance_base<STATE>& other)
	{
		this->start_        = other.start_;
		this->target_       = other.target_;
		this->instance_id_  = instance_counter_++;
		this->verbose_      = other.verbose_;
		this->extra_params_ = other.extra_params_;
		return *this;
	}

	void
	print(std::ostream& out) const
	{
		out << "problem instance[" << typeid(typename STATE::tag).name()
		    << "]; start = " << start_.id << " " << " target " << target_.id
		    << " " << " search_id " << instance_id_;
	}

	STATE start_;
	STATE target_;
	uint32_t instance_id_;
	bool verbose_;

	// stuff we might want to pass in
	void* extra_params_;
};

using problem_instance        = problem_instance_base<pack_id>;
using search_problem_instance = problem_instance_base<pad_id>;

template<class Domain>
search_problem_instance
convert_problem_instance_to_search(const problem_instance& pi, Domain& d)
{
	// convert start/target to padded/unpadded, preserving max value
	pad_id start  = pi.start_ == pack_id::max() ? pad_id::max()
	                                            : d.to_padded_id(pi.start_);
	pad_id target = pi.target_ == pack_id::max() ? pad_id::max()
	                                             : d.to_padded_id(pi.target_);
	return search_problem_instance{
	    start, target, pi.instance_id_, pi.verbose_, pi.extra_params_};
}

} // namespace warthog::search

std::ostream&
operator<<(std::ostream& str, const warthog::search::problem_instance& pi);
std::ostream&
operator<<(
    std::ostream& str, const warthog::search::search_problem_instance& pi);

#endif // WARTHOG_SEARCH_PROBLEM_INSTANCE_H
