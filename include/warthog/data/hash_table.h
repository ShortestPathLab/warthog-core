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
#include <algorithm>
#include <tuple>
#include <stdexcept>

namespace warthog::data
{

using hash_table_size_type = uint32_t;
struct bucket_lookup_pow2;
struct bucket_lookup_prime;


template <typename Key, typename Value, size_t BucketSize>
struct GroupBucket
{
	using size_type = hash_table_size_type;

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
	size_type size() const noexcept
	{
		if constexpr (size_in_chain())
		{
			return static_cast<size_type>(chain.value<size_bit_width()>());
		} else {
			return static_cast<size_type>(used);
		}
	}
	void size(size_type v) const noexcept
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

template <typename Key, typename Value, size_t BucketSize>
struct GroupBucketIterator : std::conditional_t<!std::is_const_v<Value>,
	std::tuple<GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int>, // non-const
	std::tuple<const GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int> // const
	>
{
	static consteval bool is_const() noexcept { return std::is_const_v<Value>; }
	using bucket_type = std::conditional_t<!std::is_const_v<Value>,
	std::tuple<GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int>, // non-const
	std::tuple<const GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int> // const
	>;
	using size_type = hash_table_size_type;

	using bucket_type::bucket_type;

	const bucket_type* bucket() const noexcept
	{
		return std::get<0>(*this);
	}
	bucket_type* bucket_mutable() const noexcept requires(!is_const())
	{
		return std::get<0>(*this);
	}

	size_type index() const noexcept
	{
		return std::get<1>(*this);
	}
	const Key& key() const noexcept
	{
		assert(bucket() != nullptr);
		return bucket()->key[index()];
	}
	Value& value() const noexcept
	{
		assert(bucket() != nullptr);
		return bucket()->value[index()];
	}

	operator GroupBucketIterator<Key, std::add_const_t<Value>, BucketSize>() const noexcept
	{
		return {std::get<0>(*this), std::get<1>(*this)};
	}

	bool operator==(std::nullptr_t) const noexcept
	{
		return std::get<0>(*this) == nullptr;
	}
	bool operator!=(std::nullptr_t) const noexcept
	{
		return std::get<0>(*this) != nullptr;
	}
	bool overflow() const noexcept
	{
		return std::get<1>(*this) >= BucketSize;
	}
};

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
	using size_type = hash_table_size_type;
	using key_type = Key;
	using mapped_type = Value;
	using hasher = Hash;
	using group_bucket = GroupBucket<Key, Value, BucketSize>;

	using iterator = GroupBucketIterator<Key, Value, BucketSize>;
	using const_iterator = GroupBucketIterator<Key, std::add_const_t<Value>, BucketSize>;
	using reference = Value&;
	using const_reference = std::add_const_t<Value>&;
	using pointer = Value*;
	using const_pointer = std::add_const_t<Value>*;


	Hash hash() const noexcept
	{
		return hash_;
	}
	
	template <typename K>
	size_type bucket(const K& key)
	{
		return buckets_size_.index(hash_(key));
	}
	size_type hash_to_bucket(uint64_t hash_value)
	{
		return buckets_size_.index(have_value);
	}
	
	size_type bucket_size(size_type bucket_id)
	{
		if (bucket_id < buckets_size_.size())
			return 0;
		size_type c = 0;
		for (GroupBucket* g = buckets_[bucket_id]; g != nullptr; g = g->next())
		{
			c += g->size();
		}
		return c;
	}

	template <typename K>
	iterator find(const K& key)
	{
		return find_key(key, buckets_[bucket(key)]);
	}
	template <typename K>
	const_iterator find(const K& key) const
	{
		return find_key(key, buckets_[bucket(key)]);
	}

	template <typename K>
	bool contains(const K& key) const
	{
		return find_key(key, buckets_[bucket(key)]) != nullptr;
	}
	template <typename K>
	size_type count(const K& key) const
	{
		return static_cast<size_type>(find_key(key, buckets_[bucket(key)]) != nullptr);
	}

	template <typename K>
	reference at(const K& key)
	{
		iterator it = find_key(key, buckets_[bucket(key)]);
		if (it == nullptr) {
			throw std::out_of_range("key not found");
		}
		return it.value();
	}
	template <typename K>
	const_reference at(const K& key) const
	{
		const_iterator it = find_key(key, buckets_[bucket(key)]);
		if (it == nullptr) {
			throw std::out_of_range("key not found");
		}
		return it.value();
	}

	template <typename K>
	pointer find_ptr(const K& key)
	{
		iterator it = find_key(key, buckets_[bucket(key)]);
		return it.bucket_mutable() != nullptr
			? it.bucket_mutable()->value.data() + it.index()
			: nullptr;
	}
	template <typename K>
	const_iterator find_ptr(const K& key) const
	{
		const_iterator it = find_key(key, buckets_[bucket(key)]);
		return it.bucket() != nullptr
			? it.bucket()->value.data() + it.index()
			: nullptr;
	}

	template <typename P>
	iterator emplace(Key&& key, P&& value)
	{
		iterator bucket = find_empty_key(key);
		if (bucket == nullptr || bucket.overflow())
		{
			// new bucket needed
			
		}
	}

protected:
	template <typename K>
	iterator find_key(const K& key, GroupBucket* start) const
	{
		while (start != nullptr)
		{
			size_type items = start->size();
			assert(items > 0);
			[[assume(items > 0)]];
			int index = 0;
			for (auto k_it = start->key.data(); index < items; ++index)
			{
				if (key == k_it[index]) {
					return iterator(start, index);
				}
			}
			// no items found, goto next group
			start = start->next();
		}
		return iterator(nullptr, 0);
	}
	
	iterator find_empty_key(const Key& key, GroupBucket* start) const
	{
		iterator pos(nullptr, 0);
		while (start != nullptr)
		{
			std::get<0>(pos) = start;
			size_type items = start->size();
			assert(items > 0);
			[[assume(items > 0)]];
			std::get<1>(pos) = 0;
			for (auto k_it = start->key.data(); std::get<1>(pos) < items; ++std::get<1>(pos))
			{
				if (key == k_it[index]) {
					return pos;
				}
			}
			// no items found, goto next group
			start = start->next();
		}
		return pos;
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
