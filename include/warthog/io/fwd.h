#ifndef WARTHOG_IO_FWD_H
#define WARTHOG_IO_FWD_H

/// @file fwd.h
///
/// Forward class definitions and store global enums.
///
/// @author: Ryan Hechenberger
/// @created: 2026-05-01

#include <cstdint>

namespace warthog::io
{

// serializers
class serialize_base;
class bittable_serialize;
class scenario_serialize;

// observers
class stream_observer;
class posthoc_trace;

/// @brief scenario supported version enum
enum class scenario_version : uint8_t
{
	UNKNOWN,
	VERSION_1,
	VERSION_2,
};

/// @brief named scenario v2 cost
enum class cost_type : uint8_t
{
	G_8C_NCC,
	G_8C_CC,
	G_4C,
	AA_NCC,
	AA_CC,
	OTHER,
};

/// @brief the type of bittable to (de)serialize
enum class bittable_type : uint8_t
{
	OCTILE, ///< original MovingAI format
	PATCH,  ///< patch format, grouping multiple octiles
	OTHER,  ///< unknown format
	NONE,   ///< no format specified
};

/// @brief the cell character, as specified by MovingAI
enum class gridmap_cell : char
{
	TERRAIN         = '.',
	TERRAIN_2       = 'G',
	OUT_OF_BOUNDS   = '@',
	OUT_OF_BOUNDS_2 = 'O',
	TREES           = 'T',
	SWAMP           = 'S',
	WATER           = 'W',
};

} // namespace warthog::io

#endif // WARTHOG_IO_FWD_H
