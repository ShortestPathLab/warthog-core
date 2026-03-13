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
#include <concepts>
#include <cstdint>
#include <memory>
#include <tuple>
#include <warthog/search/search_node.h>

namespace warthog::memory
{

namespace node_pool_ns
{
constexpr uint64_t LOG2_NBS = 6; // node block size = 2^n, n >= 3
constexpr uint64_t NBS      = 1 << LOG2_NBS;
constexpr uint64_t NBS_MASK = NBS - 1;
static_assert(LOG2_NBS >= 3, "must be at least 3 for size of 8");
}

class node_pool
{
public:
	struct data_deleter_ptr
	{
		void (*del)(void*) = nullptr;

		constexpr data_deleter_ptr() noexcept = default;
		constexpr data_deleter_ptr(void (*p)(void*)) noexcept : del(p) { }

		void
		operator()(void* data) const noexcept
		{
			(*del)(data);
		}
	};
	node_pool();
	node_pool(size_t num_nodes);
	~node_pool();

	template<
	    std::derived_from<search::search_node> T = search::search_node,
	    typename... NodeArgs>
	void
	set_type(NodeArgs... node_args) noexcept
	{
		clear();
		using arg_type = std::tuple<NodeArgs...>;
		if constexpr(sizeof...(NodeArgs) != 0)
		{
			create_block_data_ = static_cast<void*>(
			    new arg_type(std::forward<NodeArgs&&>(node_args)...));
		}
		block_type_sizes_ = sizeof(T);
		size_t block_sz   = node_pool_ns::NBS * sizeof(T);
		blockspool_       = std::make_unique<cpool>(block_sz, 1);
		create_block_     = [](void* block_p, sn_id_t block_id,
                           void* data) noexcept {
            assert(block_p != nullptr);
            T* block          = reinterpret_cast<T*>(block_p);
            pad_id current_id = pad_id{block_id << node_pool_ns::LOG2_NBS};
            for(uint32_t i = 0; i < node_pool_ns::NBS; ++i, ++current_id.id)
            {
                if constexpr(sizeof...(NodeArgs) == 0)
                {
                    std::construct_at(block + i, current_id);
                }
                else
                {
                    assert(data != nullptr);
                    std::apply(
                        [block_i = block + i,
                         current_id]<typename... NodeX>(NodeX&&... args) {
                            std::construct_at(block_i, current_id, args...);
                        },
                        *static_cast<arg_type*>(data));
                }
            }
		};
		clear_ = [](node_pool& np) noexcept {
			if(np.blocks_)
			{
				for(size_t i = 0; i < np.num_blocks_; ++i)
				{
					T* nodes = reinterpret_cast<T*>(np.blocks_[i]);
					if(nodes != nullptr)
					{
						for(uint32_t j = 0; j < node_pool_ns::NBS; ++j)
						{
							std::destroy_at(nodes + j);
						}
					}
				}
			}
			if constexpr(sizeof...(NodeArgs) == 0)
			{
				if(!np.create_block_data_)
				{
					arg_type* args
					    = static_cast<arg_type*>(np.create_block_data_);
					delete args;
				}
			}
		};
	}

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

	// reset nodes
	void
	clear();

private:
	void
	init(size_t nblocks);
	void
	release();

	size_t num_blocks_ = 0;
	std::unique_ptr<std::byte*[]> blocks_;
	size_t block_type_sizes_ = 0;
	std::unique_ptr<cpool> blockspool_;
	void (*create_block_)(void* block, sn_id_t bock_id, void* data) noexcept
	    = nullptr;
	void (*clear_)(node_pool& np) = nullptr;
	void* create_block_data_;
	//        uint64_t* node_init_;
	//        uint64_t node_init_sz_;
};

} // namespace warthog::memory

#endif // WARTHOG_MEMORY_NODE_POOL_H
