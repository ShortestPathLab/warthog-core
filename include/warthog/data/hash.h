#ifndef WARTHOG_DATA_HASH_H
#define WARTHOG_DATA_HASH_H

/// @file grid_trace.h
///
/// Hash functions for use in hash tables.
///
/// @author Ryan Hechenberger
/// @created 2026-09-04

#include <warthog/util/intrin.h>

#include <cstddef>
#include <cstdint>
#include <concepts>
#include <functional>

namespace warthog::data
{

struct identity_hash
{
	template <std::integral I>
	constexpr size_t operator()(I val) const noexcept
	{
		return static_cast<value_type>(
			static_cast<std::make_unsigned_t<I>>(val)
		);
	}
};

template <typename Fallback = identity_hash>
struct aes_hash
#if WARTHOG_INTRIN_HAS(SSE2)
{

};
#else
	: Fallback
{
	using Fallback::Fallback;
};
#endif

} // namespace warthog::data

#endif // WARTHOG_DATA_HASH_H
