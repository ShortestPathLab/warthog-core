#include <warthog/memory/node_pool.h>
#include <warthog/search/search_node.h>
#include <warthog/util/helpers.h>

namespace warthog::memory
{

node_pool::node_pool() = default;
node_pool::node_pool(size_t num_nodes)
{
	init(num_nodes);
	set_type<search::search_node>();
}

node_pool::~node_pool()
{
	clear();
}

void
node_pool::init(size_t num_nodes)
{
	num_blocks_ = ((num_nodes) >> node_pool_ns::LOG2_NBS) + 1;
	blocks_     = std::make_unique<std::byte*[]>(num_blocks_);
	for(size_t i = 0; i < num_blocks_; i++)
	{
		blocks_[i] = 0;
	}

	// by default:
	// allocate one chunk of memory of size
	// DEFAULT_CHUNK_SIZE and assign addresses
	// from that pool in order to generate blocks of nodes. when the pool is
	// full, cpool pre-allocates more, one chunk at a time.
}

search::search_node*
node_pool::generate(pad_id node_id)
{
	sn_id_t block_id = sn_id_t{node_id} >> node_pool_ns::LOG2_NBS;
	sn_id_t list_id  = sn_id_t{node_id} & node_pool_ns::NBS_MASK;
	assert(block_id < num_blocks_);

	// add a new block of nodes if necessary
	if(!blocks_[block_id])
	{
		// std::cerr << "generating block: "<<block_id<<std::endl;
		blocks_[block_id]
		    = reinterpret_cast<std::byte*>(blockspool_->allocate());
		create_block_(blocks_[block_id], block_id, create_block_data_);
	}

	// return the node from its position in the assocated block
	return reinterpret_cast<search::search_node*>(
	    blocks_[block_id] + list_id * block_type_sizes_);
}

void
node_pool::release()
{
	num_blocks_   = 0;
	blocks_       = nullptr;
	blockspool_   = nullptr;
	create_block_ = nullptr;
}

search::search_node*
node_pool::get_ptr(pad_id node_id)
{
	assert((sn_id_t{node_id} >> node_pool_ns::LOG2_NBS) < num_blocks_);
	sn_id_t block_id = static_cast<sn_id_t>(node_id) >> node_pool_ns::LOG2_NBS;
	sn_id_t list_id  = static_cast<sn_id_t>(node_id) & node_pool_ns::NBS_MASK;
	assert(block_id < num_blocks_);
	assert(blocks_[block_id] != nullptr);
	return reinterpret_cast<search::search_node*>(
	    blocks_[block_id] + list_id * block_type_sizes_);
}

size_t
node_pool::mem()
{
	size_t bytes
	    = sizeof(*this) + blockspool_->mem() + num_blocks_ * sizeof(void*);

	return bytes;
}

void
node_pool::clear()
{
	if(clear_) { (*clear_)(*this); }
	blockspool_        = nullptr;
	create_block_      = nullptr;
	clear_             = nullptr;
	create_block_data_ = nullptr;
}

} // namespace warthog::memory
