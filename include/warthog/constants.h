#ifndef WARTHOG_CONSTANTS_H
#define WARTHOG_CONSTANTS_H

// #include <warthog/constants.h>
//
// @author: dharabor
// @created: 01/08/2012
//

#include <bit>
#include <cassert>
#include <cfloat>
#include <climits>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>

namespace warthog
{

using sn_id_t               = uint64_t; // address space for state identifiers
constexpr sn_id_t SN_ID_MAX = std::numeric_limits<sn_id_t>::max();
constexpr sn_id_t NO_PARENT = SN_ID_MAX;

template<class Tag, std::unsigned_integral IdType = sn_id_t>
struct identity_base
{
	using id_type = IdType;
	using tag     = Tag;
	IdType id;
	identity_base() noexcept = default;
	constexpr explicit identity_base(IdType id_) noexcept : id(id_) { }
	template<typename IdType2>
	    requires(!std::same_as<IdType, IdType2>)
	explicit(
	    std::numeric_limits<IdType>::max() < std::numeric_limits<IdType2>::
	        max()) constexpr identity_base(identity_base<Tag, IdType2> alt)
	    : id(static_cast<IdType>(alt.id))
	{
		assert(id == alt.id || (is_none() && alt.is_none()));
	}
	constexpr identity_base(const identity_base&) noexcept = default;
	constexpr identity_base(identity_base&&) noexcept      = default;
	constexpr identity_base&
	operator=(const identity_base&) noexcept
	    = default;
	constexpr identity_base&
	operator=(identity_base&&) noexcept
	    = default;

	constexpr bool
	operator==(const identity_base& other) const noexcept
	{
		return id == other.id;
	}

	constexpr explicit
	operator uint64_t() const noexcept
	{
		return static_cast<uint64_t>(id);
	}
	constexpr explicit
	operator uint32_t() const noexcept
	{
		return static_cast<uint32_t>(id);
	}
	consteval static identity_base
	max() noexcept
	{
		return identity_base{std::numeric_limits<IdType>::max()};
	}
	consteval static identity_base
	none() noexcept
	{
		return max();
	}
	constexpr bool
	is_none() const noexcept
	{
		return (*this) == none();
	}

	consteval static identity_base
	zero() noexcept
	{
		return identity_base{0};
	}
	constexpr bool
	is_zero() const noexcept
	{
		return id == 0;
	}
};
template<class T>
constexpr bool is_identity_v = std::false_type{};
template<class Tag, class IdType>
constexpr bool is_identity_v<identity_base<Tag, IdType>> = std::true_type{};
template<class T>
concept Identity = is_identity_v<T>;

struct pack_tag
{ };
using pack_id   = identity_base<pack_tag>;
using pack32_id = identity_base<pack_tag, uint32_t>;
struct pad_tag
{ };
using pad_id   = identity_base<pad_tag>;
using pad32_id = identity_base<pad_tag, uint32_t>;

// each node in a weighted grid map uses sizeof(dbword) memory.
// in a uniform-cost grid map each dbword is a contiguous set
// of nodes s.t. every bit represents a node.
using dbword = uint8_t;

// gridmap constants
constexpr uint32_t DBWORD_BITS      = sizeof(warthog::dbword) * 8;
constexpr uint32_t DBWORD_BITS_MASK = (warthog::DBWORD_BITS - 1);
constexpr uint32_t LOG2_DBWORD_BITS = std::popcount(DBWORD_BITS_MASK);

// search and sort constants
constexpr double DBL_ONE = 1.0;
constexpr double DBL_TWO = 2.0;
constexpr double DBL_ROOT_TWO
    = 1.414213562373095048801688724209698078569671875;
constexpr double DBL_ONE_OVER_TWO = 0.5;
constexpr double DBL_ONE_OVER_ROOT_TWO
    = 0.70710678118654752440084436210484903928483593768847403658833986;
constexpr double DBL_ROOT_TWO_OVER_FOUR
    = 0.35355339059327376220042218105242451964241796884423701829416993;
constexpr int32_t ONE = 100000;

constexpr uint32_t INF32
    = std::numeric_limits<uint32_t>::max(); // indicates uninitialised or
                                            // undefined values
constexpr uint64_t INFTY
    = std::numeric_limits<uint64_t>::max(); // indicates uninitialised or
                                            // undefined values

using cost_t              = double;
constexpr cost_t COST_MAX = std::numeric_limits<cost_t>::max();
constexpr cost_t COST_MIN = std::numeric_limits<cost_t>::max();

// hashing constants
constexpr uint32_t FNV32_offset_basis = 2166136261;
constexpr uint32_t FNV32_prime        = 16777619;

constexpr double DIMACS_RATIO = 1e6;

}

#endif // WARTHOG_CONSTANTS_H
