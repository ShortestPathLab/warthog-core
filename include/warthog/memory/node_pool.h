#ifndef WARTHOG_MEMORY_NODE_POOL_H
#define WARTHOG_MEMORY_NODE_POOL_H

// memory/node_pool.h
//
// A memory pool of warthog::search_node objects.
//
// This implementation uses ragged two-dimensional array
// allocator. Memory for the pool is reserved but nodes
// are allocated in blocks of size NBS.
// If a node from a block needs to be geneated then the
// entire block is allocated at the same time.
// Once allocated, memory is not released again until
// destruction.
//
// On the one hand, this approach stores successor nodes in
// close proximity to their parents. On the other hand,
// blocks which are adjacent spatially may not be located
// contiguously in memory.
//
// @author: dharabor
// @created: 02/09/2012
// @updated: 2018-11-01
//

#include "cpool.h"
#include <warthog/search/search_node.h>

#include <stdint.h>

namespace warthog::memory
{

namespace node_pool_ns
{
static const uint64_t NBS      = 8; // node block size; set this >= 8
static const uint64_t LOG2_NBS = 3;
static const uint64_t NBS_MASK = 7;
}

class node_pool
{
public:
	node_pool(size_t num_nodes);
	~node_pool();

	// return a warthog::search_node object corresponding to the given id.
	// if the node has already been generated, return a pointer to the
	// previous instance; otherwise allocate memory for a new object.
	search::search_node*
	generate(pad_id node_id);

	// return a pre-allocated pointer. if the corresponding node has not
	// been allocated yet, return null
	search::search_node*
	get_ptr(pad_id node_id);

	size_t
	mem();

private:
	void
	init(size_t nblocks);

	size_t num_blocks_;
	search::search_node** blocks_;
	cpool* blockspool_;
	//        uint64_t* node_init_;
	//        uint64_t node_init_sz_;
};

} // namespace warthog::memory

#endif // WARTHOG_MEMORY_NODE_POOL_H
