#ifndef WARTHOG_IO_POSTHOC_TRACE_H
#define WARTHOG_IO_POSTHOC_TRACE_H

// io/posthoc_trace.h
//
// stream_observer that outputs a trace for use with posthoc visuliser.
// See https://posthoc-app.pathfinding.ai/
//
// @author: Ryan Hechenberger
// @created: 2025-08-07
//

#include "stream_observer.h"

namespace warthog::io
{

/// @brief base posthoc observer class.
///
/// Set the search_id to set which search the trace is printed for, by default
/// will not trace. event begin_search and end_search will setup the trace to
/// print only a specified. Inherit to create new trace format by overriding
/// print_posthoc_header for custom header. Add own events to print posthoc
/// event to stream() if (*this) holds true,
/// (*this) holds true iff id == search_id for event begin_search once, and
/// ends at end_search.
class posthoc_trace : public stream_observer
{
public:
	// DO NOT INHERIT steam_observer constructors, must be set through stream()

	/// @brief override print the header if not already printed
	virtual void
	print_posthoc_header()
	{
		if(*this)
		{
			stream() << R"posthoc(version: 1.4.0
	events:
	)posthoc";
		}
	}
	
	/// @brief override stream setting (not virtual) to print header
	/// @param stream 
	void
	open(std::ostream& stream) noexcept
	{
		stream_observer::open(stream);
		if (static_cast<stream_observer&>(*this)) {
			// print posthoc header on setting the stream
			print_posthoc_header();
		}
	}
};

} // namespace warthog::io

#endif // WARTHOG_IO_POSTHOC_TRACE_H
