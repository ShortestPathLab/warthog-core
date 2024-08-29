#include <warthog/search/gridmap_expansion_policy.h>
#include <warthog/search/problem_instance.h>
#include <warthog/util/helpers.h>

namespace warthog::search
{

gridmap_expansion_policy::gridmap_expansion_policy(
    domain::gridmap* map, bool manhattan)
    : expansion_policy(map->height() * map->width()), map_(map),
      manhattan_(manhattan)
{ }

void
gridmap_expansion_policy::expand(
    search_node* current, search_problem_instance* problem)
{
	reset();

	// get terrain type of each tile in the 3x3 square around (x, y)
	uint32_t tiles = 0;
	uint32_t nodeid = uint32_t{current->get_id()};
	map_->get_neighbours(nodeid, (uint8_t*)&tiles);

	//	#ifndef NDEBUG
	//	uint32_t cx_, cy_;
	//	warthog::helpers::index_to_xy(nodeid, map_->width(), cx_, cy_);
	//	assert(tiles[0] == map_->get_label(cx_-1, cy_-1));
	//	assert(tiles[1] == map_->get_label(cx_, cy_-1));
	//	assert(tiles[2] == map_->get_label(cx_+1, cy_-1));
	//	assert(tiles[3] == map_->get_label(cx_-1, cy_));
	//	assert(tiles[4] == map_->get_label(cx_, cy_));
	//	assert(tiles[5] == map_->get_label(cx_+1, cy_));
	//	assert(tiles[6] == map_->get_label(cx_-1, cy_+1));
	//	assert(tiles[7] == map_->get_label(cx_, cy_+1));
	//	assert(tiles[8] == map_->get_label(cx_+1, cy_+1));
	//	#endif

	// NB: no corner cutting or squeezing between obstacles!
	uint32_t nid_m_w = nodeid - map_->width();
	uint32_t nid_p_w = nodeid + map_->width();

	// generate cardinal moves
	if((tiles & 514) == 514) // N
	{
		add_neighbour(this->generate(pad_id{nid_m_w}), 1);
	}
	if((tiles & 1536) == 1536) // E
	{
		add_neighbour(this->generate(pad_id{nodeid + 1}), 1);
	}
	if((tiles & 131584) == 131584) // S
	{
		add_neighbour(this->generate(pad_id{nid_p_w}), 1);
	}
	if((tiles & 768) == 768) // W
	{
		add_neighbour(this->generate(pad_id{nodeid - 1}), 1);
	}
	if(manhattan_) { return; }

	// generate diagonal moves
	if((tiles & 1542) == 1542) // NE
	{
		add_neighbour(
		    this->generate(pad_id{nid_m_w + 1}), warthog::DBL_ROOT_TWO);
	}
	if((tiles & 394752) == 394752) // SE
	{
		add_neighbour(
		    this->generate(pad_id{nid_p_w + 1}), warthog::DBL_ROOT_TWO);
	}
	if((tiles & 197376) == 197376) // SW
	{
		add_neighbour(
		    this->generate(pad_id{nid_p_w - 1}), warthog::DBL_ROOT_TWO);
	}
	if((tiles & 771) == 771) // NW
	{
		add_neighbour(
		    this->generate(pad_id{nid_m_w - 1}), warthog::DBL_ROOT_TWO);
	}
}

search_problem_instance
gridmap_expansion_policy::get_problem_instance(problem_instance* pi)
{
	assert(pi != nullptr);
	return convert_problem_instance_to_search(*pi, *map_);
}

pack_id
gridmap_expansion_policy::get_state(pad_id node_id)
{
	return map_->to_unpadded_id(node_id);
}

pad_id
gridmap_expansion_policy::unget_state(pack_id node_id)
{
	return map_->to_padded_id(node_id);
}

void
gridmap_expansion_policy::get_xy(pack_id node_id, int32_t& x, int32_t& y)
{
	uint32_t lx, ly;
	map_->to_unpadded_xy(node_id, lx, ly);
	x = lx;
	y = ly;
}
void
gridmap_expansion_policy::get_xy(pad_id node_id, int32_t& x, int32_t& y)
{
	uint32_t lx, ly;
	map_->to_unpadded_xy(node_id, lx, ly);
	x = lx;
	y = ly;
}

void
gridmap_expansion_policy::print_node(search_node* n, std::ostream& out)
{
	uint32_t x, y;
	map_->to_unpadded_xy(n->get_id(), x, y);
	out << "(" << x << ", " << y << ")...";
	n->print(out);
}

search_node*
gridmap_expansion_policy::generate_start_node(search_problem_instance* pi)
{
	uint32_t max_id = map_->width() * map_->height();
	if(uint32_t{pi->start_} >= max_id) { return 0; }
	if(map_->get_label(uint32_t{pi->start_}) == 0) { return 0; }
	return generate(pi->start_);
}

search_node*
gridmap_expansion_policy::generate_target_node(search_problem_instance* pi)
{
	uint32_t max_id = map_->width() * map_->height();
	if((uint32_t)pi->target_ >= max_id) { return 0; }
	if(map_->get_label(uint32_t{pi->target_}) == 0) { return 0; }
	return generate(pi->target_);
}

size_t
gridmap_expansion_policy::mem()
{
	return expansion_policy::mem() + sizeof(*this) + map_->mem();
}

} // warthog::expansion_policy
