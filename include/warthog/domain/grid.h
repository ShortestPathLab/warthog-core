#ifndef WARTHOG_DOMAIN_GRID_H
#define WARTHOG_DOMAIN_GRID_H

// domains/grid.h
//
// A place for grid-related curios
//
// @author: dharabor
// @created: 2018-11-03
//

#include <warthog/constants.h>
#include <warthog/util/intrin.h>
#include <bit>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace warthog::grid
{

// TODO: document

using grid_id = pad32_id;

typedef enum : uint8_t
{
	NORTH_ID,     // 0b000
	SOUTH_ID,     // 0b001
	EAST_ID,      // 0b010
	WEST_ID,      // 0b011
	NORTHEAST_ID, // 0b100
	NORTHWEST_ID, // 0b101
	SOUTHEAST_ID, // 0b110
	SOUTHWEST_ID, // 0b111
	// only function with `secic` will consider the following second-intercardinal dirctions
	NORTH_NORTHEAST_ID = NORTH_ID | (NORTHEAST_ID << 3),
	EAST_NORTHEAST_ID = EAST_ID | (NORTHEAST_ID << 3),
	NORTH_NORTHWEST_ID = NORTH_ID | (NORTHWEST_ID << 3),
	WEST_NORTHWEST_ID = WEST_ID | (NORTHWEST_ID << 3),
	SOUTH_SOUTHEAST_ID = SOUTH_ID | (SOUTHEAST_ID << 3),
	EAST_SOUTHEAST_ID = EAST_ID | (SOUTHEAST_ID << 3),
	SOUTH_SOUTHWEST_ID = SOUTH_ID | (SOUTHWEST_ID << 3),
	WEST_SOUTHWEST_ID = WEST_ID | (SOUTHWEST_ID << 3),
} direction_id;

static_assert(NORTH_ID < 4 && SOUTH_ID < 4 && EAST_ID < 4 && WEST_ID < 4,
	"cardinals id must be less than 4");
static_assert(NORTHEAST_ID >= 4 && NORTHWEST_ID >= 4 && SOUTHEAST_ID >= 4 && SOUTHWEST_ID >= 4,
	"intercardinals id must be at least 4");
static_assert(NORTHEAST_ID < 8 && NORTHWEST_ID < 8 && SOUTHEAST_ID < 8 && SOUTHWEST_ID < 8,
	"intercardinals id must be less than 8");

template <direction_id D>
concept CardinalId = D == NORTH_ID || D == EAST_ID || D == SOUTH_ID || D == WEST_ID;
template <direction_id D>
concept InterCardinalId = D == NORTHEAST_ID || D == NORTHWEST_ID || D == SOUTHEAST_ID || D == SOUTHWEST_ID;
/// e.g. NORTH_NORTHEAST_ID
template <direction_id D>
concept SecInterCardinalID = static_cast<uint8_t>(D) >= 8;

constexpr inline bool is_cardinal_id(direction_id d) noexcept { return static_cast<uint8_t>(d) < 4; }
constexpr inline bool is_intercardinal_id(direction_id d) noexcept { return static_cast<uint8_t>(d - 4) < 4; }
constexpr inline bool is_sec_intercardinal_id(direction_id d) noexcept { return static_cast<uint8_t>(d) >= 8; }
constexpr inline direction_id secic_cardinal(direction_id d) noexcept
{
	direction_id c = static_cast<direction_id>(static_cast<uint8_t>(d) & 0b000'111);
	assert(is_cardinal_id(c));
	return c;
}
constexpr inline direction_id secic_intercardinal(direction_id d) noexcept
{
	direction_id c = static_cast<direction_id>(static_cast<uint8_t>(d) >> 3);
	assert(is_intercardinal_id(c));
	return c;
}

typedef enum : uint8_t
{
	NONE      = 0,
	NORTH     = 1 << NORTH_ID,
	EAST      = 1 << EAST_ID,
	SOUTH     = 1 << SOUTH_ID,
	WEST      = 1 << WEST_ID,
	NORTHEAST = 1 << NORTHEAST_ID,
	SOUTHEAST = 1 << SOUTHEAST_ID,
	SOUTHWEST = 1 << SOUTHWEST_ID,
	NORTHWEST = 1 << NORTHWEST_ID,
	ALL       = 255,
	CARDINAL  = NORTH | EAST | SOUTH | WEST,
	INTERCARDINAL = NORTH | EAST | SOUTH | WEST,
} direction;

template <direction D>
concept CardinalDir = D == NORTH || D == EAST || D == SOUTH || D == WEST;
template <direction D>
concept InterCardinalDir = D == NORTHEAST || D == NORTHWEST || D == SOUTHEAST || D == SOUTHWEST;

constexpr direction
to_dir(direction_id id) noexcept
{
	assert(id < 8);
	return static_cast<direction>(1 << id);
}
constexpr direction_id
to_dir_id(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	return static_cast<direction_id>(std::countr_zero(static_cast<uint8_t>(d)));
}

constexpr direction_id
get_intercardinal_hori(direction_id d) noexcept
{
	assert(is_intercardinal_id(d));
	if constexpr (NORTHEAST_ID == 4 && NORTHWEST_ID == 5 && SOUTHEAST_ID == 6 && SOUTHWEST_ID == 7) {
		// precise ordering, use math method
		if constexpr (WEST_ID == EAST_ID+1) { // EAST_ID = 2, WEST_ID = 3
			return static_cast<direction_id>(EAST_ID + (d&0b001));
		} else {
			return (d&0b001) ? WEST_ID : EAST_ID;
		}
	} else {
		constexpr uint16_t sel = ((uint16_t)(EAST_ID) << (NORTHEAST_ID << 1))
			| ((uint16_t)(EAST_ID) << (SOUTHEAST_ID << 1))
			| ((uint16_t)(WEST_ID) << (NORTHWEST_ID << 1))
			| ((uint16_t)(WEST_ID) << (SOUTHWEST_ID << 1));
		return static_cast<direction_id>((sel >> d * 2) & 0b11);
	}
}

constexpr direction_id
get_intercardinal_vert(direction_id d) noexcept
{
	assert(is_intercardinal_id(d));
	if constexpr (NORTHEAST_ID == 4 && NORTHWEST_ID == 5 && SOUTHEAST_ID == 6 && SOUTHWEST_ID == 7) {
		// precise ordering, use math method
		if constexpr (SOUTH_ID == NORTH_ID+1) { // EAST_ID = 2, WEST_ID = 3
			return static_cast<direction_id>(NORTH_ID + ((d&0b010)>>1));
		} else {
			return (d&0b010) ? SOUTH_ID : NORTH_ID;
		}
	} else {
		constexpr uint16_t sel = ((uint16_t)(NORTH_ID) << (NORTHEAST_ID << 1))
			| ((uint16_t)(SOUTH_ID) << (SOUTHEAST_ID << 1))
			| ((uint16_t)(NORTH_ID) << (NORTHWEST_ID << 1))
			| ((uint16_t)(SOUTH_ID) << (SOUTHWEST_ID << 1));
		return static_cast<direction_id>((sel >> d * 2) & 0b11);
	}
}

// rotate direction cw
constexpr direction_id
dir_id_cw90(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (WEST_ID << 2))
	    | ((uint32_t)(EAST_ID) << (NORTH_ID << 2))
	    | ((uint32_t)(SOUTH_ID) << (EAST_ID << 2))
	    | ((uint32_t)(WEST_ID) << (SOUTH_ID << 2))
	    | ((uint32_t)(NORTHEAST_ID) << (NORTHWEST_ID << 2))
	    | ((uint32_t)(SOUTHEAST_ID) << (NORTHEAST_ID << 2))
	    | ((uint32_t)(SOUTHWEST_ID) << (SOUTHEAST_ID << 2))
	    | ((uint32_t)(NORTHWEST_ID) << (SOUTHWEST_ID << 2));
	return static_cast<direction_id>((sel >> d * 4) & 0b1111);
}
constexpr direction
dir_cw90(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel = ((uint64_t)(NORTH) << (WEST_ID << 3))
	    | ((uint64_t)(EAST) << (NORTH_ID << 3))
	    | ((uint64_t)(SOUTH) << (EAST_ID << 3))
	    | ((uint64_t)(WEST) << (SOUTH_ID << 3))
	    | ((uint64_t)(NORTHEAST) << (NORTHWEST_ID << 3))
	    | ((uint64_t)(SOUTHEAST) << (NORTHEAST_ID << 3))
	    | ((uint64_t)(SOUTHWEST) << (SOUTHEAST_ID << 3))
	    | ((uint64_t)(NORTHWEST) << (SOUTHWEST_ID << 3));
	int index = std::countr_zero(static_cast<uint16_t>(
	    d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index * 8);
}

constexpr direction_id
dir_id_cw45(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (NORTHWEST_ID << 2))
	    | ((uint32_t)(EAST_ID) << (NORTHEAST_ID << 2))
	    | ((uint32_t)(SOUTH_ID) << (SOUTHEAST_ID << 2))
	    | ((uint32_t)(WEST_ID) << (SOUTHWEST_ID << 2))
	    | ((uint32_t)(NORTHEAST_ID) << (NORTH_ID << 2))
	    | ((uint32_t)(SOUTHEAST_ID) << (EAST_ID << 2))
	    | ((uint32_t)(SOUTHWEST_ID) << (SOUTH_ID << 2))
	    | ((uint32_t)(NORTHWEST_ID) << (WEST_ID << 2));
	return static_cast<direction_id>((sel >> d * 4) & 0b1111);
}
constexpr direction
dir_cw45(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel = ((uint64_t)(NORTH) << (NORTHWEST_ID << 3))
	    | ((uint64_t)(EAST) << (NORTHEAST_ID << 3))
	    | ((uint64_t)(SOUTH) << (SOUTHEAST_ID << 3))
	    | ((uint64_t)(WEST) << (SOUTHWEST_ID << 3))
	    | ((uint64_t)(NORTHEAST) << (NORTH_ID << 3))
	    | ((uint64_t)(SOUTHEAST) << (EAST_ID << 3))
	    | ((uint64_t)(SOUTHWEST) << (SOUTH_ID << 3))
	    | ((uint64_t)(NORTHWEST) << (WEST_ID << 3));
	int index = std::countr_zero(static_cast<uint16_t>(
	    d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index * 8);
}

// rotate direction ccw
constexpr direction_id
dir_id_ccw90(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (EAST_ID << 2))
	    | ((uint32_t)(EAST_ID) << (SOUTH_ID << 2))
	    | ((uint32_t)(SOUTH_ID) << (WEST_ID << 2))
	    | ((uint32_t)(WEST_ID) << (NORTH_ID << 2))
	    | ((uint32_t)(NORTHEAST_ID) << (SOUTHEAST_ID << 2))
	    | ((uint32_t)(SOUTHEAST_ID) << (SOUTHWEST_ID << 2))
	    | ((uint32_t)(SOUTHWEST_ID) << (NORTHWEST_ID << 2))
	    | ((uint32_t)(NORTHWEST_ID) << (NORTHEAST_ID << 2));
	return static_cast<direction_id>((sel >> d * 4) & 0b1111);
}
constexpr direction
dir_ccw90(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel = ((uint64_t)(NORTH) << (EAST_ID << 3))
	    | ((uint64_t)(EAST) << (SOUTH_ID << 3))
	    | ((uint64_t)(SOUTH) << (WEST_ID << 3))
	    | ((uint64_t)(WEST) << (NORTH_ID << 3))
	    | ((uint64_t)(NORTHEAST) << (SOUTHEAST_ID << 3))
	    | ((uint64_t)(SOUTHEAST) << (SOUTHWEST_ID << 3))
	    | ((uint64_t)(SOUTHWEST) << (NORTHWEST_ID << 3))
	    | ((uint64_t)(NORTHWEST) << (NORTHEAST_ID << 3));
	int index = std::countr_zero(static_cast<uint16_t>(
	    d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index * 8);
}

constexpr direction_id
dir_id_ccw45(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (NORTHEAST_ID << 2))
	    | ((uint32_t)(EAST_ID) << (SOUTHEAST_ID << 2))
	    | ((uint32_t)(SOUTH_ID) << (SOUTHWEST_ID << 2))
	    | ((uint32_t)(WEST_ID) << (NORTHWEST_ID << 2))
	    | ((uint32_t)(NORTHEAST_ID) << (EAST_ID << 2))
	    | ((uint32_t)(SOUTHEAST_ID) << (SOUTH_ID << 2))
	    | ((uint32_t)(SOUTHWEST_ID) << (WEST_ID << 2))
	    | ((uint32_t)(NORTHWEST_ID) << (NORTH_ID << 2));
	return static_cast<direction_id>((sel >> d * 4) & 0b1111);
}
constexpr direction
dir_ccw45(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel = ((uint64_t)(NORTH) << (NORTHEAST_ID << 3))
	    | ((uint64_t)(EAST) << (SOUTHEAST_ID << 3))
	    | ((uint64_t)(SOUTH) << (SOUTHWEST_ID << 3))
	    | ((uint64_t)(WEST) << (NORTHWEST_ID << 3))
	    | ((uint64_t)(NORTHEAST) << (EAST_ID << 3))
	    | ((uint64_t)(SOUTHEAST) << (SOUTH_ID << 3))
	    | ((uint64_t)(SOUTHWEST) << (WEST_ID << 3))
	    | ((uint64_t)(NORTHWEST) << (NORTH_ID << 3));
	int index = std::countr_zero(static_cast<uint16_t>(
	    d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index * 8);
}

// flip/rotate 180
constexpr direction_id
dir_id_flip(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint32_t sel = ((uint32_t)(NORTH_ID) << (SOUTH_ID << 2))
	    | ((uint32_t)(EAST_ID) << (WEST_ID << 2))
	    | ((uint32_t)(SOUTH_ID) << (NORTH_ID << 2))
	    | ((uint32_t)(WEST_ID) << (EAST_ID << 2))
	    | ((uint32_t)(NORTHEAST_ID) << (SOUTHWEST_ID << 2))
	    | ((uint32_t)(SOUTHEAST_ID) << (NORTHWEST_ID << 2))
	    | ((uint32_t)(SOUTHWEST_ID) << (NORTHEAST_ID << 2))
	    | ((uint32_t)(NORTHWEST_ID) << (SOUTHEAST_ID << 2));
	return static_cast<direction_id>((sel >> d * 4) & 0b1111);
}
constexpr direction
dir_flip(direction d) noexcept
{
	assert(std::popcount(static_cast<uint8_t>(d)) == 1);
	constexpr uint64_t sel = ((uint64_t)(NORTH) << (SOUTH_ID << 3))
	    | ((uint64_t)(EAST) << (WEST_ID << 3))
	    | ((uint64_t)(SOUTH) << (NORTH_ID << 3))
	    | ((uint64_t)(WEST) << (EAST_ID << 3))
	    | ((uint64_t)(NORTHEAST) << (SOUTHWEST_ID << 3))
	    | ((uint64_t)(SOUTHEAST) << (NORTHWEST_ID << 3))
	    | ((uint64_t)(SOUTHWEST) << (NORTHEAST_ID << 3))
	    | ((uint64_t)(NORTHWEST) << (SOUTHEAST_ID << 3));
	int index = std::countr_zero(static_cast<uint16_t>(
	    d | 256u)); // |256u to ensure no branch in compiler
	return static_cast<direction>(sel >> index * 8);
}

/// @brief TODO
/// @param d 
/// @param width 
/// @return 
constexpr uint32_t dir_id_adj(direction_id d, uint32_t width) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	int32_t w = static_cast<int32_t>(width);
	switch (d) {
	case NORTH_ID:
		return static_cast<uint32_t>(-w);
	case SOUTH_ID:
		return static_cast<uint32_t>(w);
	case EAST_ID:
		return 1u;
	case WEST_ID:
		return -1u;
	case NORTHEAST_ID:
		return static_cast<uint32_t>(-w + 1);
	case NORTHWEST_ID:
		return static_cast<uint32_t>(-w - 1);
	case SOUTHEAST_ID:
		return static_cast<uint32_t>(w + 1);
	case SOUTHWEST_ID:
		return static_cast<uint32_t>(w - 1);
	default:
		assert(false);
		return 0;
	}
}
/// @brief TODO
/// @param d 
/// @param width 
/// @return 
constexpr uint32_t dir_id_adj_inv_intercardinal(direction_id d, uint32_t a) noexcept
{
	assert(static_cast<uint8_t>(d - 4) < 4);
	switch (d) {
	case NORTHEAST_ID:
		return static_cast<uint32_t>(-static_cast<int32_t>(a) + 1);
	case NORTHWEST_ID:
		return static_cast<uint32_t>(-static_cast<int32_t>(a) - 1);
	case SOUTHEAST_ID:
		return static_cast<uint32_t>(a - 1);
	case SOUTHWEST_ID:
		return static_cast<uint32_t>(a + 1);
	default:
		assert(false);
		return 0;
	}
}
/// @brief TODO
/// @param d 
/// @param width 
/// @return 
constexpr uint32_t dir_id_adj_vert(direction_id d, uint32_t width) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	int32_t w = static_cast<int32_t>(width);
	switch (d) {
	case NORTH_ID:
		return static_cast<uint32_t>(-w);
	case SOUTH_ID:
		return static_cast<uint32_t>(w);
	case EAST_ID:
		return 0;
	case WEST_ID:
		return 0;
	case NORTHEAST_ID:
		return static_cast<uint32_t>(-w);
	case NORTHWEST_ID:
		return static_cast<uint32_t>(-w);
	case SOUTHEAST_ID:
		return static_cast<uint32_t>(w);
	case SOUTHWEST_ID:
		return static_cast<uint32_t>(w);
	default:
		assert(false);
		return 0;
	}
}
/// @brief TODO
/// @param d 
/// @param width 
/// @return 
constexpr int32_t dir_id_adj_hori(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	constexpr uint64_t sel =
	      ((uint64_t)uint8_t(0u)  << (NORTH_ID << 3))
	    | ((uint64_t)uint8_t(0u)  << (SOUTH_ID << 3))
	    | ((uint64_t)uint8_t(1u)  << (EAST_ID << 3))
	    | ((uint64_t)uint8_t(-1u) << (WEST_ID << 3))
	    | ((uint64_t)uint8_t(1u)  << (NORTHEAST_ID << 3))
	    | ((uint64_t)uint8_t(-1u) << (NORTHWEST_ID << 3))
	    | ((uint64_t)uint8_t(1u)  << (SOUTHEAST_ID << 3))
	    | ((uint64_t)uint8_t(-1u) << (SOUTHWEST_ID << 3));
	return static_cast<int32_t>( static_cast<int8_t>( static_cast<uint8_t>(sel >> (d << 3)) ) );
}

struct alignas(uint32_t) point
{
	uint16_t x;
	uint16_t y;
};

/// @brief signed point, due to point allowing >2^15 numbers, does not support point - point operations
struct alignas(uint32_t) spoint
{
	int16_t x;
	int16_t y;
};

constexpr inline std::pair<int32_t,int32_t> point_signed_diff(point a, point b) noexcept
{
	return {
		static_cast<int32_t>(static_cast<int32_t>(b.x) - static_cast<int32_t>(a.x)),
		static_cast<int32_t>(static_cast<int32_t>(b.y) - static_cast<int32_t>(a.y))
	};
}

constexpr inline point operator+(point a, spoint b) noexcept
{
	return point{static_cast<uint16_t>(a.x + static_cast<uint16_t>(b.x)), static_cast<uint16_t>(a.y + static_cast<uint16_t>(b.y))};
}
constexpr inline spoint operator+(spoint a, spoint b) noexcept
{
	return spoint{static_cast<int16_t>(a.x + b.x), static_cast<int16_t>(a.y + b.y)};
}
constexpr inline spoint operator*(int16_t a, spoint b) noexcept
{
	return spoint{static_cast<int16_t>(a * b.x), static_cast<int16_t>(a * b.y)};
}

constexpr inline spoint dir_unit_point(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	union {
		uint32_t v;
		spoint p;
	} res;
	// store point in 2 bits (10 = 1, 01 = 0, 00 = -1), then subtract 1 to pad
	// packed_reldir stores each direction (8) in 4 bits, 0b3210 as 0byyxx
	constexpr uint32_t packed_reldir =
		(0b0001 << NORTH_ID*4) |
		(0b1001 << SOUTH_ID*4) |
		(0b0110 << EAST_ID*4) |
		(0b0100 << WEST_ID*4) |
		(0b0010 << NORTHEAST_ID*4) |
		(0b0000 << NORTHWEST_ID*4) |
		(0b1010 << SOUTHEAST_ID*4) |
		(0b1000 << SOUTHWEST_ID*4);
#if WARTHOG_INTRIN_HAS(BMI2)
	res.v = _pdep_u32(packed_reldir >> d*4, 0xC000'C000u);
#else
	res.p.x = (packed_reldir >> d*4) & 0b11;
	res.p.y = (packed_reldir >> (d*4+2)) & 0b11;
#endif
	res.p.x -= 1;
	res.p.y -= 1;
	return res.p;
}
constexpr inline spoint dir_unit_point_secic(direction_id d) noexcept
{
	assert(static_cast<uint8_t>(d) < 8);
	union {
		uint32_t v;
		spoint p;
	} res;
	// store point in 2 bits (10 = 1, 01 = 0, 00 = -1), then subtract 1 to pad
	// packed_reldir stores each direction (8) in 4 bits, 0b3210 as 0byyxx
	constexpr uint32_t packed_reldir =
		(0b0001 << NORTH_ID*4) |
		(0b1001 << SOUTH_ID*4) |
		(0b0110 << EAST_ID*4) |
		(0b0100 << WEST_ID*4) |
		(0b0010 << NORTHEAST_ID*4) |
		(0b0000 << NORTHWEST_ID*4) |
		(0b1010 << SOUTHEAST_ID*4) |
		(0b1000 << SOUTHWEST_ID*4);
	
	// for second-intercardinal, return the intercardinal
	d = d < 8 ? d : secic_intercardinal(d);
#if WARTHOG_INTRIN_HAS(BMI2)
	res.v = _pdep_u32(packed_reldir >> d*4, 0xC000'C000u);
#else
	res.p.x = (packed_reldir >> d*4) & 0b11;
	res.p.y = (packed_reldir >> (d*4+2)) & 0b11;
#endif
	res.p.x -= 1;
	res.p.y -= 1;
	return res.p;
}


constexpr inline direction_id
point_to_direction_id(point p1, point p2) noexcept
{
	union {
		struct {
			int32_t x;
			int32_t y;
		} p;
		uint64_t xy;
	} c;
	c.p.x = static_cast<int32_t>(p2.x) - static_cast<int32_t>(p1.x);
	c.p.y = static_cast<int32_t>(p2.y) - static_cast<int32_t>(p1.y);

	if (c.p.x == 0) {
		return c.p.y >= 0 ? SOUTH_ID : NORTH_ID;
	} else if (c.p.y == 0) {
		return c.p.x >= 0 ? EAST_ID : WEST_ID;
	} else {
		// shift>> to mulitple of 4 (0b100)
		// (x < 0) = 0b0100
		// (y < 0) = 0b1000
		int shift = ( (static_cast<uint32_t>(c.p.x) >> (31-2)) & 0b0100 ) |
			( (static_cast<uint32_t>(c.p.y) >> (31-3)) & 0b1000 );
		assert((c.p.x > 0 && c.p.y > 0 && shift == 0)
			|| (c.p.x < 0 && c.p.y > 0 && shift == 4)
			|| (c.p.x > 0 && c.p.y < 0 && shift == 8)
			|| (c.p.x < 0 && c.p.y < 0 && shift == 12));
		return static_cast<direction_id>(
			static_cast<uint8_t>(
				static_cast<uint16_t>(
					(static_cast<uint16_t>(SOUTHEAST_ID) << 0) |
					(static_cast<uint16_t>(SOUTHWEST_ID) << 4) |
					(static_cast<uint16_t>(NORTHEAST_ID) << 8) |
					(static_cast<uint16_t>(NORTHWEST_ID) << 12)
				) >> shift
			) & 0b1111
		);
	}
}

} // namespace warthog::grid

#endif // WARTHOG_DOMAIN_GRID_H
