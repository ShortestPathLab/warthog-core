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

namespace warthog::data
{

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
template <typename Key, typename Value, size_t GroupBucket = 2, typename Hash = identity_hash, memory::alloc::ByteFactory UpstreamFactory = memory::alloc::malloc_factory>
class hash_table
{
public:
	struct Bucket
	{
		Key key;
		uint32_t index;
		uint32_t chain;
	};

protected:
	UpstreamFactory base_factory;
	memory::alloc::indexed_block_factory_type<Bucket, memory::alloc::make_factory_pointer<UpstreamFactory>, memory::alloc::void_factory, 0> bucket_list;
	memory::alloc::reuse_adaptor<
		memory::alloc::indexed_block_factory_type<Bucket, memory::alloc::make_factory_pointer<UpstreamFactory>, memory::alloc::void_factory, 0>
	> bucket_list;
};

} // namespace warthog::data

#endif // WARTHOG_DATA_HASH_TABLE_H
