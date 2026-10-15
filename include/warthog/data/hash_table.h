#ifndef WARTHOG_DATA_HASH_TABLE_H
#define WARTHOG_DATA_HASH_TABLE_H

/// @file hash_table.h
///
/// Basic hash table supprot.
///
/// @author Ryan Hechenberger
/// @created 2026-10-07

#include "hash.h"

#include <warthog/memory/alloc/block_factory.h>
#include <warthog/memory/alloc/factory_pointer.h>
#include <warthog/memory/alloc/factory_adaptor.h>
#include <warthog/memory/alloc/source_factory.h>
#include <warthog/memory/alloc/slab_factory.h>
#include <warthog/memory/alloc/object.h>
#include <warthog/util/numeric.h>
#include <warthog/util/cast.h>
#include <warthog/constants.h>

#include <type_traits>
#include <algorithm>
#include <tuple>
#include <stdexcept>
#include <span>
#include <memory>

namespace warthog::data
{

using hash_table_size_type = uint32_t;
struct bucket_lookup_pow2;
struct bucket_lookup_prime;


template <typename Key, typename Value, size_t BucketSize>
struct GroupBucket
{
	static_assert(BucketSize > 0, "BucketSize cannot be 0");
	using size_type = hash_table_size_type;

	static consteval size_t size_bit_width() noexcept { return std::bit_width(BucketSize); }
	static consteval bool size_in_chain() noexcept { return BucketSize != 1 && BucketSize < alignof(GroupBucket*); }
	
	util::pointer_tagging_lsb chain; ///< chain to next GroupBucket, might include key size, access with next()
	[[no_unique_address]] std::conditional_t<BucketSize == 1 || size_in_chain(), empty, util::fit_unsigned_t<BucketSize+1>>
		used; ///< amount of keys in use, will try to store in chain if able, access with size()
	std::array<memory::alloc::object_bytes<Key>, BucketSize> key_;
	std::array<memory::alloc::object_bytes<Value>, BucketSize> value_;

	void set(GroupBucket* next_bucket, size_type size)
	{
		if constexpr (size_in_chain())
		{
			return chain.set(next_bucket, size);
		} else { // BucketSize == 1 || size in used
			return chain.set(next_bucket);
			if constexpr (BucketSize != 1) {
				used = static_cast<decltype(used)>(size);
			}
		}
	}

	GroupBucket* next() const noexcept
	{
		if constexpr (size_in_chain())
		{
			return static_cast<GroupBucket*>(chain.ptr<size_bit_width()>());
		} else { // BucketSize == 1 || size in used
			return static_cast<GroupBucket*>(chain.notag());
		}
	}
	void next(GroupBucket* next_bucket) noexcept
	{
		if constexpr (size_in_chain())
		{
			chain.ptr<size_bit_width()>(next_bucket);
		} else { // BucketSize == 1 || size in used
			return chain.set(next_bucket);
		}
	}

	size_type size() const noexcept
	{
		if constexpr (size_in_chain())
		{
			return static_cast<size_type>(chain.value<size_bit_width()>());
		} else if constexpr (BucketSize != 1) {
			return static_cast<size_type>(used);
		} else {
			return 1;
		}
	}
	void size(size_type v) noexcept
	{
		if constexpr (size_in_chain())
		{
			chain.value<size_bit_width()>(v);
		} else if constexpr (BucketSize != 1) {
			used = static_cast<decltype(used)>(v);
		}
	}

	void size_mod(std::make_signed_t<size_type> v) noexcept
	{
		if constexpr (size_in_chain())
		{
			chain.data += static_cast<std::make_signed_t<util::pointer_tagging_lsb::value_type>> (v);
		} else if constexpr (BucketSize != 1) {
			used = used + static_cast<decltype(used)>( static_cast<std::make_signed_t<decltype(used)>>(v) );
		}
	}

	std::span<Key, BucketSize> key() noexcept
	{
		return std::span<Key, BucketSize>(reinterpret_cast<Key*>(key_.data()), BucketSize);
	}
	std::span<const Key, BucketSize> key() const noexcept
	{
		return std::span<const Key, BucketSize>(reinterpret_cast<const Key*>(key_.data()), BucketSize);
	}

	std::span<Value, BucketSize> value() noexcept
	{
		return std::span<Value, BucketSize>(reinterpret_cast<Value*>(value_.data()), BucketSize);
	}
	std::span<const Value, BucketSize> value() const noexcept
	{
		return std::span<const Value, BucketSize>(reinterpret_cast<const Value*>(value_.data()), BucketSize);
	}
};

template <typename Key, typename Value, size_t BucketSize>
struct GroupBucketIterator : std::conditional_t<!std::is_const_v<Value>,
	std::tuple<GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int>, // non-const
	std::tuple<const GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int> // const
	>
{
	static consteval bool is_const() noexcept { return std::is_const_v<Value>; }
	using tuple_type = std::conditional_t<!std::is_const_v<Value>,
	std::tuple<GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int>, // non-const
	std::tuple<const GroupBucket<Key, std::remove_const_t<Value>, BucketSize>*, int> // const
	>;
	using bucket_type = std::conditional_t<!std::is_const_v<Value>,
	GroupBucket<Key, std::remove_const_t<Value>, BucketSize>, // non-const
	const GroupBucket<Key, std::remove_const_t<Value>, BucketSize> // const
	>;
	using size_type = hash_table_size_type;

	using tuple_type::tuple_type;

	const bucket_type* bucket() const noexcept
	{
		return std::get<0>(*this);
	}
	bucket_type* bucket_mutable() const noexcept requires(!is_const())
	{
		return std::get<0>(*this);
	}
	void bucket(bucket_type* bucket) noexcept
	{
		std::get<0>(*this) = bucket;
	}

	size_type index() const noexcept
	{
		return std::get<1>(*this);
	}
	void index(size_type i) noexcept
	{
		std::get<1>(*this) = i;
	}

	const Key& key() const noexcept
	{
		assert(bucket() != nullptr);
		return bucket()->key()[index()];
	}
	const Value& value() const noexcept
	{
		assert(bucket() != nullptr);
		return bucket()->value()[index()];
	}
	Value& value() noexcept
	{
		assert(bucket() != nullptr);
		return bucket_mutable()->value()[index()];
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
		assert(bucket() != nullptr);
		return std::get<1>(*this) >= bucket()->size();
	}
	bool overflow_full() const noexcept
	{
		assert(bucket() != nullptr);
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
template <typename Key, typename Value, typename Hash = identity_hash, size_t BucketSize = 4, typename Lookup = bucket_lookup_prime, size_t Rolling = 0, memory::alloc::ByteFactory UpstreamFactory = memory::alloc::malloc_factory>
class hash_table
{
	static_assert(BucketSize > 0, "BucketSize must be greater than 0");
	static_assert(Rolling == 0 || (Rolling >= 2 && Rolling <= 16), "Rolling must be [2,16] or 0");
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

	static consteval bool rolling_reset() noexcept { return Rolling != 0; }
	static consteval uint64_t rolling_mask() noexcept { return ~(~0ull << (Rolling == 0 ? 1 : Rolling)); }

	template <typename... Args>
	bool factory_setup(Args&&... args)
	{
		if (!base_factory_.setup(std::forward<Args>(args)...))
			return false;
		if (!bucket_alloc_.setup(base_factory_))
			return false;
		rolling_id_ = 1;
		return true;
	}

	void rehash(size_type amount)
	{
		create_buckets_(amount);
	}

	Hash hash() const noexcept
	{
		return hash_;
	}
	
	template <typename K>
	size_type bucket(const K& key)
	{
		return bucket_index(hash_(key));
	}
	size_type hash_to_bucket(uint64_t hash_value)
	{
		return bucket_index(hash_value);
	}
	
	size_type bucket_size(size_type bucket_id)
	{
		if (bucket_id < buckets_size_.size() || (rolling_reset() && (bucket_id & rolling_mask()) == 0))
			return 0;
		size_type c = 0;
		for (group_bucket* g = buckets_[bucket_id]; g != nullptr; g = g->next())
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
		iterator it = find_key(key, bucket_get(hash_(key)));
		if (it == nullptr) {
			throw std::out_of_range("key not found");
		}
		return it.value();
	}
	template <typename K>
	const_reference at(const K& key) const
	{
		const_iterator it = find_key(key, bucket_get(hash_(key)));
		if (it == nullptr) {
			throw std::out_of_range("key not found");
		}
		return it.value();
	}

	template <typename K>
	pointer find_ptr(const K& key)
	{
		iterator it = find_key(key, bucket_get(hash_(key)));
		return it.bucket_mutable() != nullptr
			? it.bucket_mutable()->value.data() + it.index()
			: nullptr;
	}
	template <typename K>
	const_iterator find_ptr(const K& key) const
	{
		const_iterator it = find_key(key, bucket_get(hash_(key)));
		return it.bucket() != nullptr
			? it.bucket()->value.data() + it.index()
			: nullptr;
	}

	template <typename... Args>
	iterator emplace(Key&& key, Args&&... args)
	{
		return emplace_(std::move(key), std::forward<Args>(args)...).first;
	}
	template <typename... Args>
	iterator emplace(const Key& key, Args&&... args)
	{
		return emplace_(Key(key), std::forward<Args>(args)...).first;
	}

	template <typename... Args>
	std::pair<iterator, bool> try_emplace(Key&& key, Args&&... args)
	{
		return emplace_(std::move(key), std::forward<Args>(args)...);
	}
	template <typename... Args>
	std::pair<iterator, bool> try_emplace(const Key& key, Args&&... args)
	{
		return emplace_(Key(key), std::forward<Args>(args)...);
	}

	void clear()
	{
		clear_(true);
	}

protected:
	template <typename K>
	iterator find_key(const K& key, group_bucket* start) const
	{
		while (start != nullptr)
		{
			size_type items = start->size();
			assert(items > 0 && items <= BucketSize);
			[[assume(items > 0 && items <= BucketSize)]];
			int index = 0;
			for (auto k_it = start->key().data(); index < items; ++index)
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
	
	iterator find_empty_key(const Key& key, group_bucket* start)
	{
		if constexpr (BucketSize == 1) {
			return find_key(key, start);
		} else {
			iterator empty_pos(start, 0);
			while (empty_pos.bucket() != nullptr)
			{
				size_type items = empty_pos.bucket()->size();
				assert(items > 0 && items <= BucketSize && empty_pos.index() <= items);
				[[assume(items > 0 && items <= BucketSize)]];
				for (auto k_it = empty_pos.bucket()->key().data(); empty_pos.index() < items; empty_pos.index(empty_pos.index() + 1))
				{
					if (key == k_it[empty_pos.index()]) {
						return empty_pos;
					}
				}
				// no items found, check if end
				group_bucket* next_bucket = empty_pos.bucket()->next();
				if (next_bucket == nullptr)
					break; // final bucket
				if (items < BucketSize) {
					// space in bucket, try to pull values from next bucket if able
					bucket_pull(next_bucket, empty_pos.bucket_mutable(), items);
					continue; // check all new items
				}
				empty_pos.bucket(next_bucket);
				empty_pos.index(0);
			}
			return empty_pos;
		}
	}

	bool bucket_pull(group_bucket* from, group_bucket* to, size_type to_size) requires(BucketSize > 1)
	{
		assert(to->next() == from);
		assert(to->size() == to_size && to_size < BucketSize);

		size_type from_size = from->size();
		assert(from_size > 0);
		[[assume(from_size > 0)]];
		std::make_signed_t<size_type> changed = 0;
		do {
			// take from end of from bucket(next) and put into to bucket
			--to_size;
			++changed;
			from->key()[from_size] = std::move(to->key()[to_size]);
			from->value()[from_size] = std::move(to->value()[to_size]);
			++from_size;
		} while (from_size > 0 && to_size < BucketSize);

		// update bucket sizes
		to->size_mod(changed); // added
		if (to_size == 0) {
			// to bucket now empty, delete and relink
			from->next(to->next());
			free_bucket(from);
			return true;
		} else {
			to->size_mod(-changed);
		}

		return false;
	}

	group_bucket* new_bucket()
	{
		return reinterpret_cast<group_bucket*>(bucket_alloc_.create());
	}
	void free_bucket(group_bucket* bucket)
	{
		bucket_alloc_.destroy(reinterpret_cast<std::byte*>(bucket));
	}

	uint64_t bucket_index(uint64_t hash_value) const noexcept
	{
		if constexpr (rolling_reset()) {
			return buckets_size_.index((hash_value & rolling_mask()) != 0 ? hash_value : hash_value+1);
		} else {
			return buckets_size_.index(hash_value);
		}
	}

	group_bucket** bucket_raw(uint64_t hash_value) const noexcept
	{
		assert(buckets_ != nullptr);
		return buckets_ + bucket_index(hash_value);
	}
	group_bucket* bucket_get(uint64_t hash_value) const noexcept
	{
		assert(buckets_ != nullptr);
		if constexpr (rolling_reset()) {
			group_bucket** hash_roll = buckets_ + (hash_value & ~rolling_mask());
			if (std::bit_cast<uint64_t>(*hash_roll) != rolling_id_) {
				std::fill_n(hash_roll, 1ull << Rolling, nullptr);
				*hash_roll = std::bit_cast<group_bucket*>(rolling_id_);
				return nullptr;
			}
		}
		return buckets_[bucket_index(hash_value)];
	}
	group_bucket** bucket_mutable(uint64_t hash_value) noexcept
	{
		assert(buckets_ != nullptr);
		if constexpr (rolling_reset()) {
			group_bucket** hash_roll = buckets_ + (hash_value & ~rolling_mask());
			if (std::bit_cast<uint64_t>(*hash_roll) != rolling_id_) {
				std::fill_n(hash_roll, 1ull << Rolling, nullptr);
				*hash_roll = std::bit_cast<group_bucket*>(rolling_id_);
			}
		}
		return buckets_ + bucket_index(hash_value);
	}

	template <typename... Args>
	std::pair<iterator, bool> emplace_(Key&& key, Args&&... args)
	{
		group_bucket*& bucket = *bucket_mutable(hash_(std::as_const(key)));
		iterator pos = find_empty_key(std::as_const(key), bucket);
		if (pos == nullptr || pos.overflow_full()) // make new bucket
		{
			// new bucket needed
			group_bucket* b = new_bucket();
			if (Rolling == 1 || pos == nullptr) {
				b->set(bucket, 0);
				bucket = b;
			} else {
				assert(pos != nullptr && pos.bucket()->size() == BucketSize && pos.bucket()->next() == nullptr);
				pos.bucket_mutable()->next(b);
				b->set(nullptr, 0);
			}
			pos = iterator(b, 0);
			[[assume(pos.overflow())]];
		}
		assert(pos != nullptr && !pos.overflow_full());
		if (pos.overflow()) {
			// new element
			items_count_++;
			std::construct_at(pos.bucket_mutable()->key().data() + pos.index(), std::move(key));
			std::construct_at(pos.bucket_mutable()->value().data() + pos.index(), std::forward<Args>(args)...);
			auto s1 = pos.bucket_mutable()->size();
			pos.bucket_mutable()->size_mod(1);
			auto s2 = pos.bucket_mutable()->size();
			assert(pos.bucket_mutable()->size() == pos.index()+1);
			return {pos, true};
		}
		return {pos, false};
	}

	void create_buckets_(size_type amount)
	{
		buckets_size_.set_size(amount);
		amount = buckets_amount_();
		buckets_ = reinterpret_cast<group_bucket**>(base_factory_.allocate(amount * sizeof(group_bucket*)));
		std::fill_n(buckets_, amount, nullptr);
		rolling_id_ = 1;
	}

	size_type buckets_amount_() const noexcept
	{
		size_type amount = buckets_size_.size();
		if constexpr (rolling_reset()) {
			// align
			size_type r = amount & rolling_mask();
			if (r != 0) {
				amount += (rolling_mask()+1) - r;
			}
		}
		return amount;
	}

	void clear_(bool free_buckets)
	{
		if (rolling_id_ == 0)
			return;
		if (free_buckets) {
			bucket_alloc_.reclaim();
		}
		if constexpr (rolling_reset()) {
			rolling_id_++;
		} else {
			if (buckets_) {
				std::fill_n(buckets_, buckets_amount_(), nullptr);
			}
		}
	}
	void reset_()
	{
		if (rolling_id_ == 0)
			return;
		bucket_alloc_.release(true);
		if (buckets_) {
			base_factory_.deallocate(reinterpret_cast<std::byte*>(buckets_), buckets_amount_() * sizeof(group_bucket*));
			buckets_ = nullptr;
		}
		buckets_size_.set_size(0);
	}

protected:
	using upstream_slab = memory::alloc::slab_factory<memory::alloc::make_factory_pointer<UpstreamFactory>, (sizeof(group_bucket)+32) * 128>;
	UpstreamFactory base_factory_;
	memory::alloc::reuse_adaptor<
		memory::alloc::block_factory_type<group_bucket, upstream_slab>
		> bucket_alloc_;

	uintptr_t rolling_id_ = 0; ///< rolling id value, 0=factory not init
	group_bucket** buckets_ = nullptr; ///< list of buckets
	Lookup buckets_size_ = {}; ///< number of buckets
	uint32_t items_count_ = 0; ///< number of items in the hash table
	[[no_unique_address]] Hash hash_ = {}; ///< hash function
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

	constexpr uint32_t index(std::integral auto hash) const noexcept
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

	constexpr uint32_t index(std::integral auto hash) const noexcept
	{
		assert(prime != 0);
		return static_cast<uint32_t>(hash) % prime;
	}
};

} // namespace warthog::data

#endif // WARTHOG_DATA_HASH_TABLE_H
