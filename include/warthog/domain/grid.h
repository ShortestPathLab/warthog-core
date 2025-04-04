#ifndef WARTHOG_DOMAIN_GRID_H
#define WARTHOG_DOMAIN_GRID_H

// domains/grid.h
//
// A place for grid-related curios
//
// @author: dharabor
// @created: 2018-11-03
//

#include <bit>
#include <cstdint>
#include <cassert>

namespace warthog::grid
{

typedef enum : uint8_t
{
	NORTH_ID,
	SOUTH_ID,
	EAST_ID,
	WEST_ID,
	NORTHEAST_ID,
	NORTHWEST_ID,
	SOUTHEAST_ID,
	SOUTHWEST_ID,
} direction_id;

typedef enum : uint8_t
{
	NONE = 0,
	NORTH = 1 << NORTH_ID,
	EAST = 1 << EAST_ID,
	SOUTH = 1 << SOUTH_ID,
	WEST = 1 << WEST_ID,
	NORTHEAST = 1 << NORTHEAST_ID,
	SOUTHEAST = 1 << SOUTHEAST_ID,
	SOUTHWEST = 1 << SOUTHWEST_ID,
	NORTHWEST = 1 << NORTHWEST_ID,
	ALL = 255
} direction;

// rotate direction cw
constexpr direction dir_cw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int index = std::countr_zero(static_cast<uint8_t>(d));
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (NORTHEAST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (SOUTHEAST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (SOUTHWEST_ID << 2 ));
	return static_cast<direction>((sel >> index*4) & 0b111);
}

constexpr direction dir_ccw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int index = std::countr_zero(static_cast<uint8_t>(d));
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (SOUTHEAST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (SOUTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (NORTHEAST_ID << 2 ));
	return static_cast<direction>((sel >> index*4) & 0b111);
}

constexpr direction dir_flip(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int index = std::countr_zero(static_cast<uint8_t>(d));
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (SOUTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (NORTHEAST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (SOUTHEAST_ID << 2 ));
	return static_cast<direction>((sel >> index*4) & 0b111);
}

} // namespace warthog::grid

#endif // WARTHOG_DOMAIN_GRID_H
