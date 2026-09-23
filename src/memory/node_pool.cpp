#include <warthog/memory/node_pool.h>

#include <warthog/search/search_node.h>
#include <warthog/util/helpers.h>

namespace warthog::memory
{

node_pool::node_pool(size_t num_nodes)
{
	nodes_.setup();
	init(num_nodes);
}

void
node_pool::init(size_t num_nodes)
{
	nodes_.resize(num_nodes);
	for (size_t i = 0; i < num_nodes; ++i)
	{
		auto* node = ::new (reinterpret_cast<search::search_node*>(nodes_.get(i))) search::search_node(pad_id(i));
	}
}

node_pool::~node_pool() = default;

search::search_node*
node_pool::generate(pad_id node_id)
{
	return get_ptr(node_id);
}

search::search_node*
node_pool::get_ptr(pad_id node_id)
{
	return reinterpret_cast<search::search_node*>( nodes_.get_if(node_id.id) );
}

size_t
node_pool::mem()
{
	// TODO: update mem
	size_t bytes = nodes_.size() * sizeof(search::search_node);

	return bytes;
}

} // namespace warthog::memory
