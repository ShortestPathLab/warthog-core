#ifndef WARTHOG_IO_POSTHOC_LISTENER_H
#define WARTHOG_IO_POSTHOC_LISTENER_H

// listener/posthoc_listener.h
//
// @author: Ryan Hechenberger
// @created: 2025-08-07
//

#include "stream_listener.h"

namespace warthog::io
{

/// @brief base posthoc listener class.  
class posthoc_listener : public stream_listener
{
public:
	using stream_listener::stream_listener;

	// will print the header if not already printed
	void event(const char*);

	virtual void print_posthoc_header();

	template <typename... Args>
	void begin_search(int id, Args&&...)
	{
		do_trace_ = false;
		if (done_trace_) return; // do not repeat a trace
		if (stream_listener::operator bool() && id == search_id_) {
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

#endif // WARTHOG_IO_POSTHOC_LISTENER_H
