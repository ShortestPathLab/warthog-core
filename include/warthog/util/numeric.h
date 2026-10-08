#ifndef WARTHOG_UTIL_NUMERIC_HPP
#define WARTHOG_UTIL_NUMERIC_HPP

/*
MIT License

Copyright (c) 2024 Ryan Hechenberger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace warthog::util
{

template<typename T>
struct raise_integral_level;
template<>
struct raise_integral_level<int8_t>
{
	using type = int16_t;
};
template<>
struct raise_integral_level<int16_t>
{
	using type = int32_t;
};
template<>
struct raise_integral_level<int32_t>
{
	using type = int64_t;
};
template<>
struct raise_integral_level<int64_t>
{
	using type = int64_t;
};
template<>
struct raise_integral_level<uint8_t>
{
	using type = uint16_t;
};
template<>
struct raise_integral_level<uint16_t>
{
	using type = uint32_t;
};
template<>
struct raise_integral_level<uint32_t>
{
	using type = uint64_t;
};
template<>
struct raise_integral_level<uint64_t>
{
	using type = uint64_t;
};
template<typename T>
using raise_integral_level_t = typename raise_integral_level<T>::type;

template<typename T>
struct raise_numeric_level : raise_integral_level<T>
{ };
template<>
struct raise_numeric_level<float>
{
	using type = float;
};
template<>
struct raise_numeric_level<double>
{
	using type = double;
};
template<>
struct raise_numeric_level<long double>
{
	using type = long double;
};
template<typename T>
using raise_numeric_level_t = typename raise_numeric_level<T>::type;

namespace details
{

template<std::integral I, std::integral auto V>
struct IntFits_ : std::false_type
{ };
// V is non-negative, just check un largest unsigned integer
template<std::integral I, std::integral auto V>
    requires(V >= 0)
struct IntFits_<I, V> : std::bool_constant<
                            static_cast<uint64_t>(V) <= static_cast<uint64_t>(
                                std::numeric_limits<I>::max())>
{ };
// V is negative, unsigned I is always false
template<std::integral I, std::integral auto V>
    requires(V < 0 && std::is_unsigned_v<I>)
struct IntFits_<I, V> : std::false_type
{ };
// V is negative, check using largest signed type
template<std::integral I, std::integral auto V>
    requires(V < 0 && std::is_signed_v<I>)
struct IntFits_<I, V> : std::bool_constant<
                            static_cast<int64_t>(V) >= static_cast<int64_t>(
                                std::numeric_limits<I>::min())>
{ };

};

template<typename I, auto MaxN, auto MinN = 0>
concept IntFits
    = std::integral<decltype(MaxN)> && std::integral<decltype(MinN)>
    && details::IntFits_<I, MaxN>::value && details::IntFits_<I, MaxN>::value;

template<uint64_t MaxN>
struct fit_unsigned
{
	using type = uint64_t;
};
template<uint64_t MaxN>
    requires IntFits<uint8_t, MaxN>
struct fit_unsigned<MaxN>
{
	using type = uint8_t;
};
template<uint64_t MaxN>
    requires IntFits<uint16_t, MaxN> && (!IntFits<uint8_t, MaxN>)
struct fit_unsigned<MaxN>
{
	using type = uint16_t;
};
template<uint64_t MaxN>
    requires IntFits<uint32_t, MaxN> && (!IntFits<uint16_t, MaxN>)
struct fit_unsigned<MaxN>
{
	using type = uint32_t;
};
template<uint64_t MaxN>
using fit_unsigned_t = fit_unsigned<MaxN>::type;

template<int64_t MaxN, int64_t MinN = 0>
struct fit_signed
{
	using type = int64_t;
};
template<int64_t MaxN, int64_t MinN>
    requires IntFits<int8_t, MaxN, MinN>
struct fit_signed<MaxN, MinN>
{
	using type = int8_t;
};
template<int64_t MaxN, int64_t MinN>
    requires IntFits<int16_t, MaxN, MinN> && (!IntFits<int8_t, MaxN, MinN>)
struct fit_signed<MaxN, MinN>
{
	using type = int16_t;
};
template<int64_t MaxN, int64_t MinN>
    requires IntFits<int32_t, MaxN, MinN> && (!IntFits<int16_t, MaxN, MinN>)
struct fit_signed<MaxN, MinN>
{
	using type = int32_t;
};
template<uint64_t MaxN, int64_t MinN = 0>
using fit_signed_t = fit_signed<MaxN, MinN>::type;

template<std::integral T>
bool
is_zero(T x) noexcept
{
	return x == 0;
}
template<std::floating_point T>
bool
is_zero(T x) noexcept
{
	if constexpr(std::same_as<std::remove_cvref_t<T>, float>)
	{
		return std::abs(x) < 1e-5f;
	}
	else { return std::abs(x) < 1e-8; }
}
template<typename T, std::same_as<T>... Ts>
bool
is_all_zero(T x, Ts... xs) noexcept
{
	if constexpr(std::is_integral_v<T>) { return ((x | ... | xs) == 0); }
	else { return (is_zero(x) && ... && is_zero(xs)); }
}
template<typename T, std::same_as<T>... Ts>
bool
is_any_zero(T x, Ts... xs) noexcept
{
	if constexpr(std::is_integral_v<T>) { return ((x & ... & xs) == 0); }
	else { return (is_zero(x) || ... || is_zero(xs)); }
}

} // namespace inx::numeric

#endif // WARTHOG_UTIL_NUMERIC_HPP
