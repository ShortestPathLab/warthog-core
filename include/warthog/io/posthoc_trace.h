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
/// event begin_search and end_search will setup the trace to print only a specified.
/// Inherit to create new trace format by overriding print_posthoc_header for custom header.
/// Add own events to print posthoc event to stream() if (*this) holds true,
/// (*this) holds true iff id is on search_id between begin_search and end_search the first time only.
class posthoc_trace : public stream_observer
{
public:
	using stream_observer::stream_observer;

	// override print the header if not already printed
	virtual void print_posthoc_header();

	int search_id() const noexcept { return search_id_; }
	void search_id(int sid) noexcept { search_id_ = sid; }

	template <typename... Args>
	void begin_search(int id, Args&&...)
	{
		do_trace_ = false;
		if (done_trace_) return; // do not repeat a trace
		if (stream_observer::operator bool() && id == search_id_) {
			do_trace_ = true;
			done_trace_ = true;
			print_posthoc_header();
		}
	}
	template <typename... Args>
	void end_search(Args&&...)
	{
		do_trace_ = false;
	}

	operator bool() const noexcept
	{
		return do_trace_;
	}

protected:
	int search_id_ = 0;
	bool do_trace_ = false;
	bool done_trace_ = false;
};

} // namespace warthog::io

#endif // WARTHOG_IO_POSTHOC_TRACE_H
