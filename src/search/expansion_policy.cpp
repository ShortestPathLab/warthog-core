#include <warthog/search/expansion_policy.h>

namespace warthog::search
{

expansion_policy::expansion_policy(size_t nodes_pool_size)
{
	set_nodes_pool_size(nodes_pool_size);
}

expansion_policy::~expansion_policy()
{
	free();
}

} // namespace warthog::search
