#ifndef WARTHOG_UTIL_CAST_H
#define WARTHOG_UTIL_CAST_H

// cast.h
//
// Utility file for memory value conversion.
// Include support for byte swapping over different compilers.
//
// @author: Ryan Hechenberger
// @created: 2024-10-11
//

#include <cstdint>
#include <cstring>
#include <bit>
#ifdef _MSC_VER
#include <stdlib.h>
#endif // byteswap
#include <concepts>
#include <warthog/defines.h>

namespace warthog::util
{

// Removed, use std::bit_cast
#if 0
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
#endif

inline uintptr_t
wt_to_label(double b)
{
	return std::bit_cast<uintptr_t>(b);
}

inline double
label_to_wt(uintptr_t b)
{
	return std::bit_cast<double>(b);
}

constexpr uint16_t
byteswap_u16(uint16_t value) noexcept
{
#if defined(__GNUC__) && defined(__has_builtin)                               \
    && __has_builtin(__builtin_bswap16)
	return __builtin_bswap16(value);
#elif defined(_MSC_VER)
	return _byteswap_ushort(value);
#else
	return static_cast<uint16_t>(
	    ((value & 0xFF00u) >> 8u) | ((value & 0x00FFu) << 8u));
#endif
}

constexpr uint32_t
byteswap_u32(uint32_t value) noexcept
{
#define byteswap_u32_default_return                                           \
	((value & 0xFF000000u) >> 24u) | ((value & 0x00FF0000u) >> 8u)            \
	    | ((value & 0x0000FF00u) << 8u) | ((value & 0x000000FFu) << 24u)
#if defined(__GNUC__) && defined(__has_builtin)                               \
    && __has_builtin(__builtin_bswap32)
	return __builtin_bswap32(value);
#elif defined(_MSC_VER)
	return _byteswap_ulong(value);
#else
	return static_cast<uint32_t>(
	    ((value & 0xFF000000u) >> 24u) | ((value & 0x00FF0000u) >> 8u)
	    | ((value & 0x0000FF00u) << 8u) | ((value & 0x000000FFu) << 24u));
#endif
}

constexpr uint64_t
byteswap_u64(uint64_t value) noexcept
{
#if defined(__GNUC__) && defined(__has_builtin)                               \
    && __has_builtin(__builtin_bswap64)
	return __builtin_bswap64(value);
#elif defined(_MSC_VER)
	return _byteswap_uint64(value);
#else
	return static_cast<uint64_t>(
	    ((value & 0xFF00000000000000u) >> 56u)
	    | ((value & 0x00FF000000000000u) >> 40u)
	    | ((value & 0x0000FF0000000000u) >> 24u)
	    | ((value & 0x000000FF00000000u) >> 8u)
	    | ((value & 0x00000000FF000000u) << 8u)
	    | ((value & 0x0000000000FF0000u) << 24u)
	    | ((value & 0x000000000000FF00u) << 40u)
	    | ((value & 0x00000000000000FFu) << 56u));
#endif
}

#ifdef WARTHOG_INT128_ENABLED
constexpr unsigned __int128
byteswap_u128(unsigned __int128 value) noexcept
{
	return __builtin_bswap128(value);
}
#endif

#ifndef WARTHOG_INT128_ENABLED
template<std::integral T>
#else
template<typename T>
    requires std::same_as<T, __int128> || std::same_as<T, unsigned __int128>
    || std::integral<T>
#endif
constexpr T
byteswap_auto(T value) noexcept
{
#ifdef WARTHOG_INT128_ENABLED
	if constexpr(std::same_as<T, __int128>)
	{
		return static_cast<__int128>(
		    byteswap_u128(static_cast<unsigned __int128>(value)));
	}
	else if constexpr(std::same_as<T, unsigned __int128>)
	{
		return byteswap_u128(value);
	}
	else
#endif
	    if constexpr(std::signed_integral<T>)
	{
		return static_cast<T>(
		    byteswap_auto(static_cast<std::make_unsigned_t<T>>(value)));
	}
	else
	{
		if constexpr(std::same_as<T, uint8_t>) { return value; }
		else if constexpr(std::same_as<T, uint16_t>)
		{
			return byteswap_u16(value);
		}
		else if constexpr(std::same_as<T, uint32_t>)
		{
			return byteswap_u32(value);
		}
		else { return byteswap_u64(value); }
	}
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
