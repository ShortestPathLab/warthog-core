#ifndef WARTHOG_MEMORY_TYPED_POOL_H
#define WARTHOG_MEMORY_TYPED_POOL_H

// memory/typed_pool.h
//
// A memory pool for nodes of a templated type.
//
// This in intended as a temperary pool before an allocation
// framework is added.
//
// @author: Ryan Hechenberger
// @created: 2025-10-25
//

#include "node_pool.h"

namespace warthog::memory
{

template <typename T>
class typed_pool
{
public:
	using value_type = T;

	typed_pool(size_t num_nodes);
	~typed_pool();

	// return a warthog::search_node object corresponding to the given id.
	// if the node has already been generated, return a pointer to the
	// previous instance; otherwise allocate memory for a new object.
	value_type*
	generate(pad_id node_id);

	// return a pre-allocated pointer. if the corresponding node has not
	// been allocated yet, return null
	value_type*
	get_ptr(pad_id node_id);

	size_t
	mem();

private:
	void
	init(size_t nblocks);

	size_t num_blocks_ = 0;
	std::unique_ptr<value_type*[]> blocks_;
	std::unique_ptr<cpool> blockspool_;
	//        uint64_t* node_init_;
	//        uint64_t node_init_sz_;
};

template <typename T>
typed_pool<T>::typed_pool(size_t num_nodes)
{
	init(num_nodes);
}

template <typename T>
void
typed_pool<T>::init(size_t num_nodes)
{
	num_blocks_ = ((num_nodes) >> node_pool_ns::LOG2_NBS) + 1;
	blocks_     = std::make_unique<value_type*[]>(num_blocks_);
	for(size_t i = 0; i < num_blocks_; i++)
	{
		blocks_[i] = 0;
	}

	// by default:
	// allocate one chunk of memory of size
	// DEFAULT_CHUNK_SIZE and assign addresses
	// from that pool in order to generate blocks of nodes. when the pool is
	// full, cpool pre-allocates more, one chunk at a time.
	size_t block_sz = node_pool_ns::NBS * sizeof(value_type);
	blockspool_     = std::make_unique<cpool>(block_sz, 1);
}

template <typename T>
typed_pool<T>::~typed_pool()
{
	// delete [] node_init_;

	blockspool_->reclaim();
	delete blockspool_;

	for(size_t i = 0; i < num_blocks_; i++)
	{
		if(blocks_[i] != 0)
		{
			// std::cerr << "deleting block: "<<i<<std::endl;
			blocks_[i] = 0;
		}
	}
	delete[] blocks_;
}

template <typename T>
value_type*
typed_pool<T>::generate(pad_id node_id)
{
	sn_id_t block_id = sn_id_t{node_id} >> node_pool_ns::LOG2_NBS;
	sn_id_t list_id  = sn_id_t{node_id} & node_pool_ns::NBS_MASK;

	// id outside the pool address range
	if(block_id > num_blocks_) { return 0; }

	// add a new block of nodes if necessary
	if(!blocks_[block_id])
	{
		// std::cerr << "generating block: "<<block_id<<std::endl;
		blocks_[block_id] = new(blockspool_->allocate())
		    value_type[node_pool_ns::NBS];

		// initialise memory
		sn_id_t current_id = sn_id_t{node_id} - list_id;
		for(uint32_t i = 0; i < node_pool_ns::NBS; i += 8)
		{
			new(&blocks_[block_id][i])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 1])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 2])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 3])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 4])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 5])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 6])
			    value_type(pad_id{current_id++});
			new(&blocks_[block_id][i + 7])
			    value_type(pad_id{current_id++});
		}
	}

	// return the node from its position in the assocated block
	return &(blocks_[block_id][list_id]);
}

template <typename T>
value_type*
typed_pool<T>::get_ptr(pad_id node_id)
{
	sn_id_t block_id = sn_id_t{node_id} >> node_pool_ns::LOG2_NBS;
	sn_id_t list_id  = sn_id_t{node_id} & node_pool_ns::NBS_MASK;

	// id outside the pool address range
	if(block_id > num_blocks_) { return 0; }

	if(!blocks_[block_id]) { return 0; }
	return &(blocks_[block_id][list_id]);
}

template <typename T>
size_t
typed_pool<T>::mem()
{
	size_t bytes
	    = sizeof(*this) + blockspool_->mem() + num_blocks_ * sizeof(void*);

	return bytes;
}

} // namespace warthog::memory

#endif // WARTHOG_MEMORY_NODE_POOL_H
