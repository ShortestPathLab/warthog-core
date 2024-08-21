#include <stdio.h>
#include <warthog/util/timer.h>

namespace warthog::util
{

timer::timer()
{

#ifdef OS_MAC
	start_time = 0;
	mach_timebase_info(&timebase);

#else
	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
#endif
}

double
timer::get_time_nano()
{
#ifdef OS_MAC
	uint64_t raw_time = mach_absolute_time();
	return (double)(raw_time * timebase.numer / timebase.denom);
#else
	timespec raw_time;
	clock_gettime(CLOCK_MONOTONIC, &raw_time);
	return (double)(raw_time.tv_sec * 1e09 + raw_time.tv_nsec);
#endif
}

void
timer::start()
{
#ifdef OS_MAC
	start_time = mach_absolute_time();
#else
	clock_gettime(CLOCK_MONOTONIC, &start_time);
#endif
}

double
timer::elapsed_time_nano()
{
#ifdef OS_MAC
	uint64_t elapsed_time = mach_absolute_time() - start_time;
	return (double)(elapsed_time * timebase.numer / timebase.denom);

#else
	timespec current_time;
	clock_gettime(CLOCK_MONOTONIC, &current_time);
	return (current_time.tv_sec * 1e09 + current_time.tv_nsec)
	    - (start_time.tv_sec * 1e09 + start_time.tv_nsec);
#endif
}

void
timer::reset()
{
#ifdef OS_MAC
	start_time = 0;
#else
	start_time.tv_sec = 0;
	start_time.tv_nsec = 0;
#endif
}

double
timer::elapsed_time_micro()
{
	return elapsed_time_nano() / 1000.0;
}

double
timer::elapsed_time_sec()
{
	return elapsed_time_nano() / 1e9f;
}

} // namespace warthog::util
