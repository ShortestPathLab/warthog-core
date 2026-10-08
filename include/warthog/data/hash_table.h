#ifndef WARTHOG_DATA_HASH_TABLE_H
#define WARTHOG_DATA_HASH_TABLE_H

/// @file hash_table.h
///
/// Basic hash table supprot.
///
/// @author Ryan Hechenberger
/// @created 2026-10-07

#include "hash.h"

#include <warthog/memory/alloc/indexed_block_factory.h>
#include <warthog/memory/alloc/factory_pointer.h>
#include <warthog/memory/alloc/factory_adaptor.h>
#include <warthog/memory/alloc/source_factory.h>
#include <warthog/util/numeric.h>
#include <warthog/util/cast.h>

#include <type_traits>

namespace warthog::data
{

struct bucket_lookup_pow2;
struct bucket_lookup_prime;

/// @brief a basic hash table that stores low-level classes
/// @tparam Key the key value type
/// @tparam Value to class paired with Key, should not require a destructor call otherwise memory leaks can occur
/// @tparam GroupBucket the number of keys stored in each bucket before switching to chain linking
/// @tparam Hash the hash function for Key
/// @tparam UpstreamFactory 
///
/// A simple hash table using the memory::alloc system for memory shaping.
/// Underlying structure is as follows:
/// buckets -> list[<key*GroupBucket,index*GroupBucket> + chain]
/// data -> list[Value]
///
/// Bucket uses hash to get index of bucket, and stores GroupBuckets number in bucket.
/// If an overflow of bucket occurs, 
template <typename Key, typename Value, size_t BucketSize = 4, typename Hash = identity_hash, typename Lookup = bucket_lookup_prime, memory::alloc::ByteFactory UpstreamFactory = memory::alloc::malloc_factory>
class hash_table
{
	static_assert(BucketSize > 0, "BucketSize must be greater than 0");
public:
	struct GroupBucket
	{
		static consteval bool size_bit_width() noexcept { return std::bit_width(BucketSize); }
		static consteval bool size_in_chain() noexcept { return BucketSize < alignof(GroupBucket*, Key); }
		
		util::pointer_tagging_lsb chain; ///< chain to next GroupBucket, might include key size, access with next()
		[[no_unique_address]] std::conditional_t<size_in_chain(), empty, util::fit_unsigned_t<BucketSize>>
			used; ///< amount of keys in use, will try to store in chain if able, access with size()
		std::array<Key, BucketSize> key;
		std::array<Value, BucketSize> value;

		GroupBucket* next() const noexcept
		{
			if constexpr (size_in_chain())
			{
				return chain.ptr<size_bit_width()>();
			} else {
				return chain.notag();
			}
		}
		uint32_t size() const noexcept
		{
			if constexpr (size_in_chain())
			{
				return static_cast<uint32_t>(chain.value<size_bit_width()>());
			} else {
				return static_cast<uint32_t>(used);
			}
		}
		void size(uint32_t v) const noexcept
		{
			if constexpr (size_in_chain())
			{
				chain.value<size_bit_width()>(v);
			} else {
				used = static_cast<decltype(used)>(v);
			}
		}
		void size_inc() const noexcept
		{
			if constexpr (size_in_chain())
			{
				++chain.data;
			} else {
				++used;
			}
		}
		void size_dec() const noexcept
		{
			if constexpr (size_in_chain())
			{
				--chain.data;
			} else {
				--used;
			}
		}
	};

	Hash hash() const noexcept
	{
		return hash_;
	}

protected:
	uint32_t get_bucket(uint64_t hash_value)
	{
		return buckets_size_.index(have_value);
	}
	void find_key(const Key& key, GroupBucket* start)
	{
		
	}

protected:
	UpstreamFactory base_factory_;
	memory::alloc::indexed_block_factory_type<Bucket, memory::alloc::make_factory_pointer<UpstreamFactory>, memory::alloc::void_factory, 0> bucket_list_;

	GroupBucket** buckets_; ///< list of buckets
	Lookup buckets_size_; ///< number of buckets
	uint32_t items_count_; ///< number of items in the hash table
	[[no_unique_address]] Hash hash_; ///< hash function
};


struct bucket_lookup_pow2
{
	uint32_t mask; ///< power of 2

	bucket_lookup_pow2() = default;
	constexpr bucket_lookup_pow2(uint32_t min) noexcept : mask(0)
	{
		set_size(min);
	}

	constexpr uint32_t size() const noexcept
	{
		return mask+1;
	}
	constexpr bool empty() const noexcept
	{
		return mask == 0;
	}

	constexpr uint32_t set_size(uint32_t min) noexcept
	{
		if (min == 0) {
			mask = 0;
			return 0;
		}
		uint64_t up_size = std::bit_ceil(static_cast<uint64_t>(min));
		if (up_size < 4)
			up_size = 4;
		if (--up_size > std::numeric_limits<uint32_t>::max())
			up_size = ~static_cast<uint32_t>(0);
		mask = up_size;
		return mask + 1;
	}

	constexpr uint32_t index(std::integral auto hash)
	{
		assert(mask != 0);
		return static_cast<uint32_t>(hash) & mask;
	}
};

struct bucket_lookup_prime
{
	uint32_t prime; ///< power of 2

	bucket_lookup_prime() = default;
	constexpr bucket_lookup_prime(uint32_t min) noexcept : prime(0)
	{
		set_size(min);
	}

	constexpr uint32_t size() const noexcept
	{
		return prime;
	}
	constexpr bool empty() const noexcept
	{
		return prime == 0;
	}

	constexpr uint32_t set_size(uint32_t near_min) noexcept
	{
		if (near_min == 0) {
			prime = 0;
			return 0;
		}
		// list of 2^n-k of prime numbers, where n = index+1 and k = value
		// for primes from 2^1 to 2^32
		// list found https://t5k.org/lists/2small/0bit.html
		constexpr std::array<uint8_t, 32> PRIME_2N_K{
            {0, 1, 1, 3, 1, 3, 1,  5, 3,  3, 9,  3,  1, 3,  19, 15,
            1, 5, 1, 3, 9, 3, 15, 3, 39, 5, 39, 57, 3, 35, 1,  5}};

		uint64_t up_size = std::bit_ceil(static_cast<uint64_t>(near_min));
		if (up_size < 4)
			up_size = 4;
		if (up_size > std::numeric_limits<uint32_t>::max())
			up_size = static_cast<uint64_t>(std::numeric_limits<uint32_t>::max()) + 1;
		prime = static_cast<uint32_t>( up_size - PRIME_2N_K[std::countr_zero(up_size)-1] );
		return prime;
	}

	constexpr uint32_t index(std::integral auto hash)
	{
		assert(prime != 0);
		return static_cast<uint32_t>(hash) % prime;
	}
};

} // namespace warthog::data

#endif // WARTHOG_DATA_HASH_TABLE_H
