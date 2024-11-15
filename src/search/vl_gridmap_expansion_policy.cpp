#include <warthog/search/problem_instance.h>
#include <warthog/search/vl_gridmap_expansion_policy.h>
#include <warthog/util/helpers.h>

namespace warthog::search
{

vl_gridmap_expansion_policy_base::vl_gridmap_expansion_policy_base(
    domain::vl_gridmap* map, util::cost_table& costs)
    : expansion_policy(map->height() * map->width()), map_(map), costs_(costs)
{ }

vl_gridmap_expansion_policy_base::~vl_gridmap_expansion_policy_base() { }

size_t
vl_gridmap_expansion_policy_base::mem()
{
	return expansion_policy::mem()
	    + (sizeof(vl_gridmap_expansion_policy_base) - sizeof(expansion_policy))
	    + map_->mem();
}

search_problem_instance
vl_gridmap_expansion_policy_base::get_problem_instance(problem_instance* pi)
{
	assert(pi != nullptr);
	return convert_problem_instance_to_search(*pi, *map_);
}

pack_id
vl_gridmap_expansion_policy_base::get_state(pad_id node_id)
{
	return map_->to_unpadded_id(node_id);
}

pad_id
vl_gridmap_expansion_policy_base::unget_state(pack_id node_id)
{
	return map_->to_padded_id(node_id);
}

void
vl_gridmap_expansion_policy_base::get_xy(
    pack_id node_id, int32_t& x, int32_t& y)
{
	uint32_t lx, ly;
	map_->to_unpadded_xy(node_id, lx, ly);
	x = lx;
	y = ly;
}
void
vl_gridmap_expansion_policy_base::get_xy(
    pad_id node_id, int32_t& x, int32_t& y)
{
	uint32_t lx, ly;
	map_->to_unpadded_xy(node_id, lx, ly);
	x = lx;
	y = ly;
}

void
vl_gridmap_expansion_policy_base::print_node(search_node* n, std::ostream& out)
{
	uint32_t x, y;
	map_->to_unpadded_xy(n->get_id(), x, y);
	out << "(" << x << ", " << y << ")...";
	n->print(out);
}

void
vl_gridmap_expansion_policy::expand(
    search_node* current, search_problem_instance* problem)
{
	reset();

	// ids of current tile and its 8 neighbours
	uint32_t id_node = uint32_t{current->get_id()};
	uint32_t id_N = id_node - map_->width();
	uint32_t id_S = id_node + map_->width();
	uint32_t id_E = id_node + 1;
	uint32_t id_W = id_node - 1;
	uint32_t id_NE = id_N + 1;
	uint32_t id_NW = id_N - 1;
	uint32_t id_SE = id_S + 1;
	uint32_t id_SW = id_S - 1;

	warthog::dbword* label = &map_->get_label(id_node);
	warthog::dbword* label_N = &map_->get_label(id_N);
	warthog::dbword* label_S = &map_->get_label(id_S);
	warthog::dbword* label_E = label + 1;
	warthog::dbword* label_W = label - 1;
	warthog::dbword* label_NE = label_N + 1;
	warthog::dbword* label_NW = label_N - 1;
	warthog::dbword* label_SE = label_S + 1;
	warthog::dbword* label_SW = label_S - 1;

	// generate neighbours to the north
	if(costs_[*label_N])
	{
		search_node* n = generate(pad_id{id_N});
		double cost = (costs_[*label] + costs_[*label_N]) * 0.5;
		add_neighbour(n, cost);

		if(costs_[*label_NE] && costs_[*label_E])
		{
			search_node* n = generate(pad_id{id_NE});
			double cost = (costs_[*label] + costs_[*label_N] + costs_[*label_E]
			               + costs_[*label_NE])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
		if(costs_[*label_NW] && costs_[*label_W])
		{
			search_node* n = generate(pad_id{id_NW});
			double cost = (costs_[*label] + costs_[*label_N] + costs_[*label_W]
			               + costs_[*label_NW])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
	}

	// neighburs to the south
	if(costs_[*label_S])
	{
		search_node* n = generate(pad_id{id_S});
		double cost = (costs_[*label] + costs_[*label_S]) * 0.5;
		add_neighbour(n, cost);

		if(costs_[*label_SE] && costs_[*label_E])
		{
			search_node* n = generate(pad_id{id_SE});
			double cost = (costs_[*label] + costs_[*label_S] + costs_[*label_E]
			               + costs_[*label_SE])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
		if(costs_[*label_SW] && costs_[*label_W])
		{
			search_node* n = generate(pad_id{id_SW});
			double cost = (costs_[*label] + costs_[*label_S] + costs_[*label_W]
			               + costs_[*label_SW])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
	}

	// neighbour to the east
	if(costs_[*label_E])
	{
		search_node* n = generate(pad_id{id_E});
		double cost = (costs_[*label] + costs_[*label_E]) * 0.5;
		add_neighbour(n, cost);
	}

	// neighbour to the west
	if(costs_[*label_W])
	{
		search_node* n = generate(pad_id{id_W});
		double cost = (costs_[*label] + costs_[*label_W]) * 0.5;
		add_neighbour(n, cost);
	}
}

search_node*
vl_gridmap_expansion_policy::generate_start_node(search_problem_instance* pi)
{
	uint32_t max_id = map_->width() * map_->height();
	if(uint32_t{pi->start_} >= max_id) { return 0; }
	return generate(pi->start_);
}

search_node*
vl_gridmap_expansion_policy::generate_target_node(search_problem_instance* pi)
{
	uint32_t max_id = map_->width() * map_->height();
	if(uint32_t{pi->target_} >= max_id) { return 0; }
	return generate(pi->target_);
}

size_t
vl_gridmap_expansion_policy::mem()
{
	return vl_gridmap_expansion_policy_base::mem()
	    + (sizeof(vl_gridmap_expansion_policy)
	       - sizeof(vl_gridmap_expansion_policy_base));
}

} // namespace warthog::search
