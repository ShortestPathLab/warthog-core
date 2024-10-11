#ifndef WARTHOG_SEARCH_SEARCH_NODE_H
#define WARTHOG_SEARCH_SEARCH_NODE_H

// search_node.h
//
// @author: dharabor
// @created: 10/08/2012
//

#include <warthog/constants.h>
#include <warthog/memory/cpool.h>

#include <ostream>

namespace warthog::search
{

class search_node
{
public:
	search_node(pad_id id = pad_id::max())
	    : id_(id), parent_id_(warthog::SN_ID_MAX), g_(warthog::COST_MAX),
	      f_(warthog::COST_MAX), ub_(warthog::COST_MAX), status_(0),
	      priority_(warthog::INF32), search_number_(UINT32_MAX)
	{
		refcount_++;
	}

	~search_node() { refcount_--; }

	inline void
	init(
	    uint32_t search_number, pad_id parent_id, cost_t g, cost_t f,
	    cost_t ub = warthog::COST_MAX)
	{
		parent_id_ = parent_id;
		f_ = f;
		g_ = g;
		ub_ = ub;
		search_number_ = search_number;
		status_ = false;
	}

	inline uint32_t
	get_search_number() const
	{
		return search_number_;
	}

	inline void
	set_search_number(uint32_t search_number)
	{
		search_number_ = search_number;
	}

	inline pad_id
	get_id() const
	{
		return id_;
	}

	inline void
	set_id(pad_id id)
	{
		id_ = id;
	}

	inline bool
	get_expanded() const
	{
		return status_;
	}

	inline void
	set_expanded(bool expanded)
	{
		status_ = expanded;
	}

	inline pad_id
	get_parent() const
	{
		return parent_id_;
	}

	inline void
	set_parent(pad_id parent_id)
	{
		parent_id_ = parent_id;
	}

	inline uint32_t
	get_priority() const
	{
		return priority_;
	}

	inline void
	set_priority(uint32_t priority)
	{
		priority_ = priority;
	}

	inline cost_t
	get_g() const
	{
		return g_;
	}

	inline void
	set_g(cost_t g)
	{
		g_ = g;
	}

	inline cost_t
	get_f() const
	{
		return f_;
	}

	inline void
	set_f(cost_t f)
	{
		f_ = f;
	}

	inline cost_t
	get_ub() const
	{
		return ub_;
	}

	inline void
	set_ub(cost_t ub)
	{
		ub_ = ub;
	}

	inline void
	relax(cost_t g, pad_id parent_id)
	{
		assert(g < g_);
		f_ = (f_ - g_) + g;
		g_ = g;
		if(ub_ < warthog::COST_MAX) { ub_ = (ub_ - g_) + g; }
		parent_id_ = parent_id;
	}

	inline bool
	operator<(const search_node& other) const
	{
		//    static uint64_t SIGN_MASK = UINT64_MAX & (1ULL<<63);
		//    cost_t result = this->f_ - other.f_;
		//    uint64_t sign = ((uint64_t)result) >> 63;
		//    if(!((uint64_t)result & ~SIGN_MASK))
		//    {
		//        result = g_ - other.g_;
		//        sign = (((uint64_t)result) >> 63) ^ 1ULL;
		//    }
		//    return sign;

		if(f_ < other.f_) { return true; }
		if(f_ > other.f_) { return false; }

		// break ties in favour of larger g
		if(g_ > other.g_) { return true; }
		return false;
	}

	inline bool
	operator>(const search_node& other) const
	{
		if(f_ > other.f_) { return true; }
		if(f_ < other.f_) { return false; }

		// break ties in favour of larger g
		if(g_ > other.g_) { return true; }
		return false;
	}

	inline bool
	operator==(const search_node& other) const
	{
		if(!(*this < other) && !(*this > other)) { return true; }
		return false;
	}

	inline bool
	operator<=(const search_node& other) const
	{
		if(*this < other) { return true; }
		if(!(*this > other)) { return true; }
		return false;
	}

	inline bool
	operator>=(const search_node& other) const
	{
		if(*this > other) { return true; }
		if(!(*this < other)) { return true; }
		return false;
	}

	inline void
	print(std::ostream& out) const
	{
		out << "search_node id:" << get_id().id;
		out << " p_id: ";
		out << parent_id_.id;
		out << " g: " << g_ << " f: " << this->get_f() << " ub: " << ub_
		    << " expanded: " << get_expanded() << " "
		    << " search_number_: " << search_number_;
	}

	uint32_t
	mem()
	{
		return sizeof(*this);
	}

	static uint32_t
	get_refcount()
	{
		return refcount_;
	}

private:
	pad_id id_;
	pad_id parent_id_;

	cost_t g_;
	cost_t f_;
	cost_t ub_;

	// TODO steal the high-bit from priority instead of ::status_ ?
	uint8_t status_;    // open or closed
	uint32_t priority_; // expansion priority

	uint32_t search_number_;
	static uint32_t refcount_;
};

struct cmp_less_search_node
{
	inline bool
	operator()(const search_node& first, const search_node& second)
	{
		return first < second;
	}
};

struct cmp_greater_search_node
{
	inline bool
	operator()(const search_node& first, const search_node& second)
	{
		return first > second;
	}
};

struct cmp_less_search_node_f_only
{
	inline bool
	operator()(const search_node& first, const search_node& second)
	{
		return first.get_f() < second.get_f();
	}
};

} // namespace warthog::search

std::ostream&
operator<<(std::ostream& str, const warthog::search::search_node& sn);

#endif // WARTHOG_SEARCH_SEARCH_NODE_H
