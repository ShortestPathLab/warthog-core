#ifndef WARTHOG_UTIL_COST_TABLE_H
#define WARTHOG_UTIL_COST_TABLE_H

// cost_table.h
//
// A utility for mapping terrain types (ASCII characters) to their weights
// (warthog::cost_t) in weighted maps. This is backed by a simple 256-element
// array. Cost 0 is used to represent untraversable terrain, and NaN is used as
// a "not specified" sentinel.
//
// The file format consists of one entry per line, where the first character of
// a line is the terrain type, then any amount of whitespace, followed by the
// decimal terrain cost. Example specifying `.` as traversable with cost 1 and
// `@`, `T` as untraversable: . 1
// @ 0
// T 0
//
// @author: Mark Carlson
// @created: 2022-06-30
//

#include <array>
#include <warthog/domain/labelled_gridmap.h>

namespace warthog::util
{

class cost_table
{
public:
	cost_table()
	{
		// Tiles out-of-bounds of the map have terrain type 0, so make it
		// untraversable.
		costs_[0] = 0.0;
		for(int i = 1; i < 256; i++)
		{
			costs_[i] = 0.0 / 0.0;
		}
	}

	cost_table(const char* filename);

	cost_t
	lowest_cost(domain::vl_gridmap& map);

	warthog::cost_t&
	operator[](uint8_t index)
	{
		return costs_[index];
	}

private:
	std::array<warthog::cost_t, 256> costs_;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_COST_TABLE_H
