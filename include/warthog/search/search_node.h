#ifndef WARTHOG_SEARCH_SEARCH_NODE_H
#define WARTHOG_SEARCH_SEARCH_NODE_H

// search_node.h
//
// @author: dharabor
// @created: 10/08/2012
//

#include <warthog/constants.h>
#include <warthog/io/log.h>
#include <warthog/memory/cpool.h>

#include <ostream>

namespace warthog::search
{

struct search_node
{
	search_node() noexcept = default;
	search_node(pad_id id) noexcept : id_(id) { }

	inline void
	init(
	    uint32_t search_number, pad_id parent_id, cost_t g, cost_t f,
	    cost_t ub = warthog::COST_MAX) noexcept
	{
		parent_id_     = parent_id;
		f_             = f;
		g_             = g;
		ub_            = ub;
		search_number_ = search_number;
		status_        = false;
	}

	inline uint32_t
	get_search_number() const noexcept
	{
		return search_number_;
	}

	inline void
	set_search_number(uint32_t search_number) noexcept
	{
		search_number_ = search_number;
	}

	inline pad_id
	get_id() const noexcept
	{
		return id_;
	}

	inline void
	set_id(pad_id id) noexcept
	{
		id_ = id;
	}

	inline bool
	get_expanded() const noexcept
	{
		return status_;
	}

	inline void
	set_expanded(bool expanded) noexcept
	{
		status_ = expanded;
	}

	inline pad_id
	get_parent() const noexcept
	{
		return parent_id_;
	}

	inline void
	set_parent(pad_id parent_id) noexcept
	{
		parent_id_ = parent_id;
	}

	inline uint32_t
	get_priority() const noexcept
	{
		return priority_;
	}

	inline void
	set_priority(uint32_t priority) noexcept
	{
		priority_ = priority;
	}

	inline cost_t
	get_g() const noexcept
	{
		return g_;
	}

	inline void
	set_g(cost_t g) noexcept
	{
		g_ = g;
	}

	inline cost_t
	get_f() const noexcept
	{
		return f_;
	}

	inline void
	set_f(cost_t f) noexcept
	{
		f_ = f;
	}

	inline cost_t
	get_ub() const noexcept
	{
		return ub_;
	}

	inline void
	set_ub(cost_t ub) noexcept
	{
		ub_ = ub;
	}

	inline void
	relax(cost_t g, pad_id parent_id) noexcept
	{
		assert(g < g_);
		f_ = (f_ - g_) + g;
		g_ = g;
		if(ub_ < warthog::COST_MAX) { ub_ = (ub_ - g_) + g; }
		parent_id_ = parent_id;
	}

	inline bool
	operator<(const search_node& other) const noexcept
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
	operator>(const search_node& other) const noexcept
	{
		if(f_ > other.f_) { return true; }
		if(f_ < other.f_) { return false; }

		// break ties in favour of larger g
		if(g_ > other.g_) { return true; }
		return false;
	}

	inline bool
	operator==(const search_node& other) const noexcept
	{
		if(!(*this < other) && !(*this > other)) { return true; }
		return false;
	}

	inline bool
	operator<=(const search_node& other) const noexcept
	{
		if(*this < other) { return true; }
		if(!(*this > other)) { return true; }
		return false;
	}

	inline bool
	operator>=(const search_node& other) const noexcept
	{
		if(*this > other) { return true; }
		if(!(*this < other)) { return true; }
		return false;
	}

	void
	print(std::ostream& out) const;

	uint32_t
	mem() noexcept
	{
		return sizeof(*this);
	}

	pad_id id_        = pad_id(warthog::SN_ID_MAX);
	pad_id parent_id_ = pad_id(warthog::SN_ID_MAX);

	cost_t g_  = warthog::COST_MAX;
	cost_t f_  = warthog::COST_MAX;
	cost_t ub_ = warthog::COST_MAX;

	// TODO steal the high-bit from priority instead of ::status_ ?
	uint8_t status_    = 0;              // open or closed
	uint32_t priority_ = warthog::INF32; // expansion priority

	uint32_t search_number_ = UINT32_MAX;
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

template<>
struct std::formatter<::warthog::search::search_node, char>
{
	template<typename ParseContext>
	constexpr auto
	parse(ParseContext& ctx) const
	{
		return ctx.begin();
	}

	template<class FmtContext>
	FmtContext::iterator
	format(const ::warthog::search::search_node& s, FmtContext& ctx) const
	{
		return std::format_to(
		    ctx.out(),
		    "search_node id:{} p_id:{} g:{} f:{} ub:{} expanded:{} "
		    "search_number:{}",
		    s.get_id().id, s.get_parent().id, s.get_g(), s.get_f(), s.get_ub(),
		    s.get_expanded(), s.get_search_number());
	}
};

#endif // WARTHOG_SEARCH_SEARCH_NODE_H
