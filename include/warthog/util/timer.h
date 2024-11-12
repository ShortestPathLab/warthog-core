// timer.h
//
// A cross-platform monotonic wallclock timer.
// Currently supports nanoseconds resolution.
//
// Reference doco for timers on OSX:
// https://developer.apple.com/library/mac/qa/qa1398/_index.html
// https://developer.apple.com/library/mac/technotes/tn2169/_index.html#//apple_ref/doc/uid/DTS40013172-CH1-TNTAG5000
//
// @author: dharabor
//
// @created: September 2012
//

#ifndef WARTHOG_UTIL_TIMER_H
#define WARTHOG_UTIL_TIMER_H

#include <chrono>

namespace warthog::util
{

class timer
{

public:
	using clock = std::chrono::steady_clock;

	timer();
	void
	reset();
	void
	start();
	std::chrono::nanoseconds
	elapsed_time_nano();
	double
	elapsed_time_micro()
	{
		return elapsed_time_nano().count() * 1e-3;
	}
	double
	elapsed_time_sec()
	{
		return elapsed_time_nano().count() * 1e-9;
	}
	clock::time_point
	get_time();
	// std::chrono::nanoseconds
	// get_time_nano();
	// double
	// get_time_micro()
	// {
	// 	return get_time_nano().count() * 1e-3;
	// }
	// double
	// get_time_sec()
	// {
	// 	return get_time_nano().count() * 1e-9;
	// }

private:
	clock::time_point start_time;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_TIMER_H
