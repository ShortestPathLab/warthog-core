#ifndef WARTHOG_UTIL_CAST_H
#define WARTHOG_UTIL_CAST_H

#include <cstdint>
#include <cstring>
#ifdef _MSC_VER
#include <stdlib.h>
#endif // byteswap

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

constexpr uint32_t byteswap_u32(uint32_t value) noexcept
{
#define byteswap_u32_default_return ((value & 0xFF000000u) >> 24u) | \
      ((value & 0x00FF0000u) >> 8u) | \
      ((value & 0x0000FF00u) << 8u) | \
      ((value & 0x000000FFu) << 24u)
#if defined(__GNUC__) && defined(__has_builtin)
#if __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(value);
#else
	return byteswap_u32_default_return;
#endif

#elif defined(_MSC_VER)
	return _byteswap_ulong(value);

#else
	return byteswap_u32_default_return;
#endif
#undef byteswap_u32_default_return
}

constexpr uint64_t byteswap_u64(uint64_t value) noexcept
{
#define byteswap_u64_default_return ((value & 0xFF00000000000000u) >> 56u) | \
      ((value & 0x00FF000000000000u) >> 40u) | \
      ((value & 0x0000FF0000000000u) >> 24u) | \
      ((value & 0x000000FF00000000u) >>  8u) | \
      ((value & 0x00000000FF000000u) <<  8u) | \
      ((value & 0x0000000000FF0000u) << 24u) | \
      ((value & 0x000000000000FF00u) << 40u) | \
      ((value & 0x00000000000000FFu) << 56u)
#if defined(__GNUC__) && defined(__has_builtin)
#if __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(value);
#else
	return byteswap_u64_default_return;
#endif

#elif defined(_MSC_VER)
	return _byteswap_uint64(value);

#else
	return byteswap_u64_default_return;
#endif
#undef byteswap_u64_default_return
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
