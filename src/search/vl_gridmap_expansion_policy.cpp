#include <warthog/search/problem_instance.h>
#include <warthog/search/vl_gridmap_expansion_policy.h>
#include <warthog/util/helpers.h>

namespace warthog::search
{

vl_gridmap_expansion_policy::vl_gridmap_expansion_policy(
    domain::vl_gridmap* map, util::cost_table& costs)
    : expansion_policy(map->height() * map->width()), map_(map), costs_(costs)
{ }

vl_gridmap_expansion_policy::~vl_gridmap_expansion_policy() { }

void
vl_gridmap_expansion_policy::expand(
    search_node* current, problem_instance* problem)
{
	reset();

	// ids of current tile and its 8 neighbours
	uint32_t id_node = (uint32_t)current->get_id();
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
		search_node* n = generate(id_N);
		double cost = (costs_[*label] + costs_[*label_N]) * 0.5;
		add_neighbour(n, cost);

		if(costs_[*label_NE] && costs_[*label_E])
		{
			search_node* n = generate(id_NE);
			double cost = (costs_[*label] + costs_[*label_N] + costs_[*label_E]
			               + costs_[*label_NE])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
		if(costs_[*label_NW] && costs_[*label_W])
		{
			search_node* n = generate(id_NW);
			double cost = (costs_[*label] + costs_[*label_N] + costs_[*label_W]
			               + costs_[*label_NW])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
	}

	// neighburs to the south
	if(costs_[*label_S])
	{
		search_node* n = generate(id_S);
		double cost = (costs_[*label] + costs_[*label_S]) * 0.5;
		add_neighbour(n, cost);

		if(costs_[*label_SE] && costs_[*label_E])
		{
			search_node* n = generate(id_SE);
			double cost = (costs_[*label] + costs_[*label_S] + costs_[*label_E]
			               + costs_[*label_SE])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
		if(costs_[*label_SW] && costs_[*label_W])
		{
			search_node* n = generate(id_SW);
			double cost = (costs_[*label] + costs_[*label_S] + costs_[*label_W]
			               + costs_[*label_SW])
			    * warthog::DBL_ROOT_TWO * 0.25;
			add_neighbour(n, cost);
		}
	}

	// neighbour to the east
	if(costs_[*label_E])
	{
		search_node* n = generate(id_E);
		double cost = (costs_[*label] + costs_[*label_E]) * 0.5;
		add_neighbour(n, cost);
	}

	// neighbour to the west
	if(costs_[*label_W])
	{
		search_node* n = generate(id_W);
		double cost = (costs_[*label] + costs_[*label_W]) * 0.5;
		add_neighbour(n, cost);
	}
}

uint32_t
vl_gridmap_expansion_policy::get_state(warthog::sn_id_t node_id)
{
	return map_->to_unpadded_id(node_id);
}

void
vl_gridmap_expansion_policy::get_xy(sn_id_t node_id, int32_t& x, int32_t& y)
{
	uint32_t lx, ly;
	map_->to_unpadded_xy(node_id, lx, ly);
	x = lx;
	y = ly;
}

void
vl_gridmap_expansion_policy::print_node(search_node* n, std::ostream& out)
{
	uint32_t x, y;
	map_->to_padded_xy(n->get_id(), x, y);
	out << "(" << x << ", " << y << ")...";
	n->print(out);
}

search_node*
vl_gridmap_expansion_policy::generate_start_node(problem_instance* pi)
{
	uint32_t start = (uint32_t)pi->start_;
	uint32_t max_id = map_->header_width() * map_->header_height();
	if(start >= max_id) { return 0; }
	uint32_t padded_id = map_->to_padded_id(start);
	return generate(padded_id);
}

search_node*
vl_gridmap_expansion_policy::generate_target_node(problem_instance* pi)
{
	uint32_t target = (uint32_t)pi->target_;
	uint32_t max_id = map_->header_width() * map_->header_height();

	if(target >= max_id) { return 0; }
	uint32_t padded_id = map_->to_padded_id(target);
	return generate(padded_id);
}

} // namespace warthog::search
