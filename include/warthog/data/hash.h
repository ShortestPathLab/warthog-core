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
	static constexpr bool stateful_hash() noexcept { return false; } 

	template <std::integral I>
	constexpr size_t operator()(I val) const noexcept
	{
		return static_cast<value_type>(
			static_cast<std::make_unsigned_t<I>>(val)
		);
	}
};

template <uint64_t KeyHigh = 0, uint64_t KeyLow = 0, typename Fallback = identity_hash>
struct aes_hash
#if WARTHOG_INTRIN_HAS(SSE2)
{
	static constexpr int key_mutable_parts() noexcept { return static_cast<int>(KeyHigh == 0) + static_cast<int>(KeyLow == 0); }
	static constexpr bool stateful_hash() noexcept { return key_mutable_parts() != 0; }

	void set_key(uint64_t key_part) requires(key_mutable_parts() == 1)
	{
		key_[0] = key_part;
	}
	void set_key(uint64_t key_high, uint64_t key_low) requires(key_mutable_parts() == 2)
	{
		key_[0] = key_high;
		key_[1] = key_low;
	}

	__m128i get_key() const noexcept requires (stateful_hash())
	{
		if constexpr (key_mutable_parts() == 1)
		{
			if (KeyHigh == 0) {
				return _mm_set_epi64(key_[0], KeyLow);
			} else {
				return _mm_set_epi64(KeyHigh, key_[0]);
			}
		} else {
			return _mm_set_epi64(key_[0], key_[1]);
		}
	}
	static __m128i get_key() noexcept requires (!stateful_hash())
	{
		return _mm_set_epi64(KeyHigh, KeyLow);
	}

	template <std::integral I>
	constexpr size_t operator()(I val) const noexcept
	{
		
		return static_cast<value_type>(
			static_cast<std::make_unsigned_t<I>>(val)
		);
	}

	std::array<uint64_t, key_mutable_parts()> key_ = {};
};
#else
	: Fallback
{
	using Fallback::Fallback;
};
#endif

} // namespace warthog::data

#endif // WARTHOG_DATA_HASH_H
