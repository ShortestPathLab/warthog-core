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
constexpr direction_id dir_id_cw(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel =
		((uint32_t)(NORTH_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (NORTHEAST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (SOUTHEAST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (SOUTHWEST_ID << 2 ));
	return static_cast<direction_id>((sel >> d*4) & 0b1111);
}
constexpr direction dir_cw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel =
		((uint64_t)(NORTH) << (WEST_ID << 3 ))
	|	((uint64_t)(EAST) << (NORTH_ID << 3 ))
	|	((uint64_t)(SOUTH) << (EAST_ID << 3 ))
	|	((uint64_t)(WEST) << (SOUTH_ID << 3 ))
	|	((uint64_t)(NORTHEAST) << (NORTHWEST_ID << 3 ))
	|	((uint64_t)(SOUTHEAST) << (NORTHEAST_ID << 3 ))
	|	((uint64_t)(SOUTHWEST) << (SOUTHEAST_ID << 3 ))
	|	((uint64_t)(NORTHWEST) << (SOUTHWEST_ID << 3 ));
	int index = std::countr_zero(static_cast<uint16_t>(d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index*8);
}

constexpr direction_id dir_id_ccw(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel =
		((uint32_t)(NORTH_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (SOUTHEAST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (SOUTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (NORTHEAST_ID << 2 ));
	return static_cast<direction_id>((sel >> d*4) & 0b1111);
}
constexpr direction dir_ccw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel =
		((uint64_t)(NORTH) << (EAST_ID << 3 ))
	|	((uint64_t)(EAST) << (SOUTH_ID << 3 ))
	|	((uint64_t)(SOUTH) << (WEST_ID << 3 ))
	|	((uint64_t)(WEST) << (NORTH_ID << 3 ))
	|	((uint64_t)(NORTHEAST) << (SOUTHEAST_ID << 3 ))
	|	((uint64_t)(SOUTHEAST) << (SOUTHWEST_ID << 3 ))
	|	((uint64_t)(SOUTHWEST) << (NORTHWEST_ID << 3 ))
	|	((uint64_t)(NORTHWEST) << (NORTHEAST_ID << 3 ));
	int index = std::countr_zero(static_cast<uint16_t>(d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index*8);
}

constexpr direction_id dir_id_flip(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel =
		((uint32_t)(NORTH_ID) << (SOUTH_ID << 2 ))
	|	((uint32_t)(EAST_ID) << (WEST_ID << 2 ))
	|	((uint32_t)(SOUTH_ID) << (NORTH_ID << 2 ))
	|	((uint32_t)(WEST_ID) << (EAST_ID << 2 ))
	|	((uint32_t)(NORTHEAST_ID) << (SOUTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHEAST_ID) << (NORTHWEST_ID << 2 ))
	|	((uint32_t)(SOUTHWEST_ID) << (NORTHEAST_ID << 2 ))
	|	((uint32_t)(NORTHWEST_ID) << (SOUTHEAST_ID << 2 ));
	return static_cast<direction_id>((sel >> d*4) & 0b1111);
}
constexpr direction dir_flip(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel =
		((uint64_t)(NORTH) << (SOUTH_ID << 3 ))
	|	((uint64_t)(EAST) << (WEST_ID << 3 ))
	|	((uint64_t)(SOUTH) << (NORTH_ID << 3 ))
	|	((uint64_t)(WEST) << (EAST_ID << 3 ))
	|	((uint64_t)(NORTHEAST) << (SOUTHWEST_ID << 3 ))
	|	((uint64_t)(SOUTHEAST) << (NORTHWEST_ID << 3 ))
	|	((uint64_t)(SOUTHWEST) << (NORTHEAST_ID << 3 ))
	|	((uint64_t)(NORTHWEST) << (SOUTHEAST_ID << 3 ));
	int index = std::countr_zero(static_cast<uint16_t>(d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index*8);
}

} // namespace warthog::grid

#endif // WARTHOG_DOMAIN_GRID_H
