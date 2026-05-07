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
class scenario_serialize;

// observers
class stream_observer;
class posthoc_trace;

enum class scenario_version : uint8_t
{
	UNKNOWN,
	VERSION_1,
	VERSION_2,
};
enum class cost_type : uint8_t
{
	G_8C_NCC,
	G_8C_CC,
	G_4C,
	AA_NCC,
	AA_CC,
	OTHER,
};

} // namespace warthog::io

#endif // WARTHOG_IO_FWD_H
