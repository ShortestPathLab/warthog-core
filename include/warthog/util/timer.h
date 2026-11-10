/// @file timer.h
///
/// A cross-platform monotonic wallclock timer.
/// Currently supports nanoseconds resolution.
///
/// @author dharabor, Ryan Hechenberger
/// @date 2012-10-10

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

private:
	clock::time_point start_time;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_TIMER_H
