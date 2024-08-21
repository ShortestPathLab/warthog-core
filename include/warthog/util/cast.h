#ifndef WARTHOG_UTIL_CAST_H
#define WARTHOG_UTIL_CAST_H

#include <cstdint>
#include <cstring>

namespace warthog::util
{

// Thanks Graeme
//
// Hack of reinterpret_cast
template<class T, class U>
T
conv(const U& x)
{
	static_assert(
	    sizeof(T) == sizeof(U),
	    "Should bit-cast between values of equal size");
	T ret;
	memcpy(&ret, &x, sizeof(U));
	return ret;
}

inline uintptr_t
wt_to_label(double b)
{
	return conv<uintptr_t, double>(b);
}

inline double
label_to_wt(uintptr_t b)
{
	return conv<double, uintptr_t>(b);
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
