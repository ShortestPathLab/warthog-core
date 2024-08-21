#include <warthog/search/expansion_policy.h>

namespace warthog::search
{

expansion_policy::expansion_policy(size_t nodes_pool_size)
{
	nodes_pool_size_ = nodes_pool_size;
	nodepool_ = new memory::node_pool(nodes_pool_size);
	// neis_ = new std::vector<neighbour_record>();
	// neis_->reserve(32);
	neis_ = new memory::arraylist<neighbour_record>(32);
}

expansion_policy::~expansion_policy()
{
	reset();
	delete neis_;
	delete nodepool_;
}

} // namespace warthog::search
