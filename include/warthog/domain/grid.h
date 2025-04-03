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
	NONE = 0,
	NORTH = 1,
	EAST = 2,
	SOUTH = 4,
	WEST = 8,
	NORTHEAST = 16,
	SOUTHEAST = 32,
	SOUTHWEST = 64,
	NORTHWEST = 128,
	ALL = 255
} direction;

constexpr direction dir_cw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int b = std::countr_zero(static_cast<uint8_t>(d));
	int adj = b & 0b100;
	b = (b + 1) & 0b011;
	return static_cast<direction>(1u << (b + adj));
}

constexpr direction dir_ccw(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int b = std::countr_zero(static_cast<uint8_t>(d));
	int adj = b & 0b100;
	b = (b - 1) & 0b011;
	return static_cast<direction>(1u << (b + adj));
}

constexpr direction dir_flip(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	int b = std::countr_zero(static_cast<uint8_t>(d));
	b ^= 0b010;
	return static_cast<direction>(1u << b);
}

} // namespace warthog::grid

#endif // WARTHOG_DOMAIN_GRID_H
