#ifndef WARTHOG_DATA_HASH_H
#define WARTHOG_DATA_HASH_H

/// @file hash.h
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

/// @brief hash function that returns any integer as self converted to size_t
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

/// @brief runs multiple hash functions together, starting from left to right.
/// @tparam ...Hashes 
///
/// Links hash functions together.  An example could be an aes_hash, which only supports integers.
/// A user could use chain_hash<std::hash<std::string>, aes_hash<0x42fe'3453'85ee'3322>, when passing
/// a string it will first hash with std::hash(str) then into aes_hash(size_t).
template <typename... Hashes>
	requires (sizeof...(Hashes) >= 2)
struct chain_hash : Hashes...
{
	static constexpr bool stateful_hash() noexcept { return (Hashes::stateful_hash() || ...); }

	constexpr size_t operator()(auto&& val) const noexcept
	{
		return hash_<Hashes...>(val);
	}

	template <typename H, typename... Hs>
	constexpr size_t hash_(auto&& val) const noexcept
	{
		if constexpr (sizeof...(Hs) == 0) {
			return H::operator()(val);
		} else {
			return hash_<Hs>(H::operator()(val));
		}
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
				return _mm_set_epi64x(key_[0], KeyLow);
			} else {
				return _mm_set_epi64x(KeyHigh, key_[0]);
			}
		} else {
			return _mm_set_epi64x(key_[0], key_[1]);
		}
	}
	static __m128i get_key() noexcept requires (!stateful_hash())
	{
		return _mm_set_epi64x(KeyHigh, KeyLow);
	}

	template <std::integral I>
	constexpr size_t operator()(I val) const noexcept
	{
		if constexpr (std::is_signed_v<I>) {
			return operator()(static_cast<std::make_unsigned_t<I>>(val));
		} else {
#ifdef WARTHOG_INT128
			if constexpr (sizeof(I) > 8) { // unsigned __int128_t
				alignas(unsigned __int128) uint64_t r[2];
				unsigned __int128 v = static_cast<unsigned __int128>(val);
				std::memcpy(+r, &v, std::min(sizeof(v),sizeof(r)));
				__m128i res = _mm_aesenc_si128(_mm_set_epi64x(r[0], r[1]), get_key());
				return static_cast<size_t>(_mm_cvtsi128_si64(res));	
			} else {
#endif // WARTHOG_INT128
				__m128i res = _mm_aesenc_si128(_mm_set_epi64x(static_cast<uint64_t>(val), 0), get_key());
				return static_cast<size_t>(_mm_cvtsi128_si64(res));	
#ifdef WARTHOG_INT128
			}
#endif
		}
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
