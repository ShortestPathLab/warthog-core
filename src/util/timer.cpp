#include <stdio.h>
#include <warthog/util/timer.h>

namespace warthog::util
{

timer::timer() : start_time{} { }

auto
timer::get_time() -> clock::time_point
{
	return clock::now();
}

void
timer::start()
{
	start_time = clock::now();
}

std::chrono::nanoseconds
timer::elapsed_time_nano()
{
	auto current_time = clock::now();
	return current_time - start_time;
}

void
timer::reset()
{
	start_time = {};
}

} // namespace warthog::util
