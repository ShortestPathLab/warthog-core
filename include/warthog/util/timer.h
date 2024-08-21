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

#ifdef OS_MAC
#include <mach/mach.h>
#include <mach/mach_time.h>

#else
#include <time.h>
#endif

namespace warthog::util
{

class timer
{

public:
	timer();
	void
	reset();
	void
	start();
	double
	elapsed_time_nano();
	double
	elapsed_time_micro();
	double
	elapsed_time_sec();
	double
	get_time_nano();
	inline double
	get_time_micro()
	{
		return get_time_nano() / 1000.0;
	}
	inline double
	get_time_sec()
	{
		return get_time_nano() / 1e9f;
	}

private:
#ifdef OS_MAC
	uint64_t start_time;
	mach_timebase_info_data_t timebase;
#else
	timespec start_time;
#endif
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_TIMER_H
