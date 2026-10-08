#ifndef WARTHOG_UTIL_FIXED_POINT_HPP
#define WARTHOG_UTIL_FIXED_POINT_HPP

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

#include <warthog/defines.h>

#include <algorithm>
#include <cassert>
#include <climits>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace warthog::util
{

template<size_t Digits>
struct binary_fixed_width_deduce
{
	using type                     = std::int64_t;
	static constexpr size_t digits = 64;
	static constexpr bool overflow = true;
};
// fit into i64
template<size_t Digits>
    requires(32 > Digits && Digits <= 64)
struct binary_fixed_width_deduce<Digits>
{
	using type                     = std::int64_t;
	static constexpr size_t digits = Digits;
	static constexpr bool overflow = false;
};
// fit into i32
template<size_t Digits>
    requires(16 > Digits && Digits <= 32)
struct binary_fixed_width_deduce<Digits>
{
	using type                     = std::int32_t;
	static constexpr size_t digits = Digits;
	static constexpr bool overflow = false;
};
// fit into i16
template<size_t Digits>
    requires(8 > Digits && Digits <= 16)
struct binary_fixed_width_deduce<Digits>
{
	using type                     = std::int16_t;
	static constexpr size_t digits = Digits;
	static constexpr bool overflow = false;
};
// fit into i8
template<size_t Digits>
    requires(Digits <= 8)
struct binary_fixed_width_deduce<Digits>
{
	using type                     = std::int8_t;
	static constexpr size_t digits = Digits;
	static constexpr bool overflow = false;
};

template<size_t Digits, size_t Fraction>
struct binary_fixed_point
{
	using deduce     = binary_fixed_width_deduce<Digits>;
	using value_type = typename deduce::type;
	static_assert(
	    Digits <= deduce::digits, "Digits must not exceed max integer size.");
	static_assert(
	    Fraction < deduce::digits, "Fraction must be less than integer size.");

	static consteval size_t
	digits() noexcept
	{
		return deduce::digits;
	}
	static consteval size_t
	logical_digits() noexcept
	{
		return Digits;
	}
	static consteval size_t
	frac() noexcept
	{
		return Fraction;
	}
	static consteval bool
	overflow() noexcept
	{
		return deduce::overflow;
	}

	value_type value;

	binary_fixed_point() noexcept = default;
	constexpr explicit binary_fixed_point(value_type v) noexcept : value{v} { }
	constexpr binary_fixed_point(const binary_fixed_point& v) noexcept
	    = default;
	constexpr binary_fixed_point(binary_fixed_point&& v) noexcept = default;
	template<std::floating_point FP>
	explicit binary_fixed_point(FP val) noexcept
	    : value(static_cast<FP>(val) * (1 << Fraction))
	{ }
	~binary_fixed_point() = default;

	template<std::floating_point FP>
	explicit
	operator FP() const noexcept
	{
		return static_cast<FP>(value) / (1 << Fraction);
	}
	template<std::integral Int>
	constexpr explicit
	operator Int() const noexcept
	{
		return static_cast<int>(value >> Fraction);
	}
	template<size_t OD, size_t OF>
	constexpr explicit(OD < Digits)
	operator binary_fixed_point<OD, OF>() const noexcept
	{
		using to_type  = binary_fixed_point<OD, OF>;
		using bit_type = std::conditional_t<
		    (Digits >= OD), value_type, typename to_type::value_type>;
		return to_type(static_cast<typename to_type::value_type>(
		    bit_shift(static_cast<bit_type>(value), OF - Fraction)));
	}
};

template<typename T>
concept BinaryFixedPoint
    = std::same_as<T, binary_fixed_point<T::digits(), T::frac()>>;

template<BinaryFixedPoint A, BinaryFixedPoint B>
using common_binary_fixed_point = binary_fixed_point<
    std::max(A::logical_digits(), B::logical_digits())
        + (std::max(A::frac(), B::frac()) - std::min(A::frac(), B::frac())),
    std::max(A::frac(), B::frac())>;

template<BinaryFixedPoint A, BinaryFixedPoint B>
using common_binary_fixed_point = binary_fixed_point<
    std::max(A::logical_digits(), B::logical_digits())
        + (std::max(A::frac(), B::frac()) - std::min(A::frac(), B::frac())),
    std::max(A::frac(), B::frac())>;

template<BinaryFixedPoint A, BinaryFixedPoint B>
struct multiply_binary_fixed_point
{
	using type = binary_fixed_point<
	    A::logical_digits() + B::logical_digits(), A::frac() + B::frac()>;
	constexpr static bool fits_int = true;
};
template<BinaryFixedPoint A, BinaryFixedPoint B>
    requires(A::logical_digits() + B::logical_digits() > 64)
struct multiply_binary_fixed_point<A, B>
{
	constexpr static size_t frac_total = A::frac() + B::frac();
	constexpr static size_t frac_max   = std::max(A::frac(), B::frac());
	using type                         = binary_fixed_point<
	                            A::logical_digits() + B::logical_digits(), A::frac() + B::frac()>;
	constexpr static bool fits_int = false;
};

/**
 * @return plus operator of type common_binary_fixed_point<A,B>
 */
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr auto
operator+(A a, B b) noexcept
{
	if(std::same_as<A, B>) { return A(a.value + b.value); }
	else
	{
		using type = common_binary_fixed_point<A, B>;
		using vt   = typename type::value_type;
		return type(static_cast<vt>(
		    static_cast<type>(a).value + static_cast<type>(b).value));
	}
}
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr A&
operator+=(A& a, B b) noexcept
{
	a.value += static_cast<A>(b).value;
	return a;
}

/**
 * @return plus operator of type common_binary_fixed_point<A,B>
 */
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr auto
operator-(A a, B b) noexcept
{
	if(std::same_as<A, B>) { return A(a.value - b.value); }
	else
	{
		using type = common_binary_fixed_point<A, B>;
		using vt   = typename type::value_type;
		return type(static_cast<vt>(
		    static_cast<type>(a).value - static_cast<type>(b).value));
	}
}
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr A&
operator-=(A& a, B b) noexcept
{
	a.value -= static_cast<A>(b).value;
	return a;
}

/**
 * @return plus operator of type common_binary_fixed_point<A,B>
 */
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr auto
operator*(A a, B b) noexcept
{
	using multi = multiply_binary_fixed_point<A, B>;
	if constexpr(multi::fits_int)
	{
		int64_t res
		    = static_cast<int64_t>(a.value) * static_cast<int64_t>(b.value);
		return typename multi::type(static_cast<multi::type::value_type>(res));
	}
	else
	{
#ifdef WARTHOG_INT128_ENABLED
		__int128 res = static_cast<__int128>(a.value) * b.value;
		res        >>= A::frac() + B::frac() - multi::type::frac();
		return typename multi::type(static_cast<multi::type::value_type>(res));
#else
		assert(false);
		return typename multi::type{};
#endif // WARTHOG_INT128_ENABLED
	}
}
template<BinaryFixedPoint A, BinaryFixedPoint B>
constexpr A&
operator*=(A& a, B b) noexcept
{
	using multi = multiply_binary_fixed_point<A, B>;
	if constexpr(multi::fits_int)
	{
		int64_t res
		    = static_cast<int64_t>(a.value) * static_cast<int64_t>(b.value);
		a.value = static_cast<A::value_type>(res >> B::frac());
	}
	else
	{
#ifdef WARTHOG_INT128_ENABLED
		__int128 res = static_cast<__int128>(a.value) * b.value;
		a.value      = static_cast<A::value_type>(res >> B::frac());
#else
		assert(false);
		a.value = 0;
#endif // WARTHOG_INT128_ENABLED
	}
	return a;
}

template<BinaryFixedPoint A>
constexpr A
operator>>(A a, uint32_t b) noexcept
{
	assert(b < sizeof(A::value_type) * CHAR_BIT);
	return A(static_cast<A::value_type>(a.value >> b));
}
template<BinaryFixedPoint A>
constexpr A&
operator>>=(A& a, uint32_t b) noexcept
{
	assert(b < sizeof(A::value_type) * CHAR_BIT);
	a.value >>= b;
	return a;
}
template<BinaryFixedPoint A>
constexpr A
operator<<(A a, uint32_t b) noexcept
{
	assert(b < sizeof(A::value_type) * CHAR_BIT);
	return A(static_cast<A::value_type>(a.value << b));
}
template<BinaryFixedPoint A>
constexpr A&
operator<<=(A& a, uint32_t b) noexcept
{
	assert(b < sizeof(A::value_type) * CHAR_BIT);
	a.value <<= b;
	return a;
}

template<BinaryFixedPoint A>
constexpr A
operator|(A a, A b) noexcept
{
	return A(static_cast<A::value_type>(a.value | b.value));
}
template<BinaryFixedPoint A>
constexpr A&
operator|=(A& a, A b) noexcept
{
	a.value |= b.value;
	return a;
}
template<BinaryFixedPoint A>
constexpr A
operator&(A a, A b) noexcept
{
	return A(static_cast<A::value_type>(a.value & b.value));
}
template<BinaryFixedPoint A>
constexpr A&
operator&=(A& a, A b) noexcept
{
	a.value &= b.value;
	return a;
}
template<BinaryFixedPoint A>
constexpr A
operator^(A a, A b) noexcept
{
	return A(static_cast<A::value_type>(a.value ^ b.value));
}
template<BinaryFixedPoint A>
constexpr A&
operator^=(A& a, A b) noexcept
{
	a.value ^= b.value;
	return a;
}

template<BinaryFixedPoint A>
constexpr std::strong_ordering
operator<=>(A a, A b) noexcept
{
	return a.value <=> b.value;
}
template<BinaryFixedPoint A, BinaryFixedPoint B>
    requires(!std::same_as<A, B>)
constexpr std::strong_ordering
operator<=>(A a, B b) noexcept
{
	using common = common_binary_fixed_point<A, B>;
	if constexpr(A::frac() == B::frac() || common::logical_digits() <= 64)
	{
		return static_cast<common>(a) <=> static_cast<common>(b);
	}
	else
	{
		// overflow, handle
#ifdef WARTHOG_INT128_ENABLED
		__int128 a128 = static_cast<__int128>(a.value);
		__int128 b128 = static_cast<__int128>(b.value);
		// align
		if constexpr(A::frac() > B::frac())
		{
			b128 <<= (A::frac() - B::frac());
		}
		else { a128 <<= (B::frac() - A::frac()); }
		return a128 <=> b128;
#else
		// handle
		assert(false);
		return static_cast<common>(a) <=> static_cast<common>(b);
#endif // WARTHOG_INT128_ENABLED
	}
}

} // namespace inx::numeric

namespace std
{

template<size_t Digits, size_t Fraction>
struct numeric_limits<::warthog::util::binary_fixed_point<Digits, Fraction>>
{
	using type       = ::warthog::util::binary_fixed_point<Digits, Fraction>;
	using value_type = typename type::value_type;
	static constexpr bool is_specialized                = true;
	static constexpr bool is_signed                     = true;
	static constexpr bool is_integer                    = Fraction == 0;
	static constexpr bool is_exact                      = true;
	static constexpr bool has_infinity                  = false;
	static constexpr bool has_quiet_NaN                 = false;
	static constexpr bool has_signaling_NaN             = false;
	static constexpr std::float_denorm_style has_denorm = std::denorm_absent;
	static constexpr bool has_denorm_loss               = false;
	static constexpr std::float_round_style round_style
	    = std::round_toward_zero;
	static constexpr bool is_iec559  = false;
	static constexpr bool is_bounded = true;
	static constexpr bool is_modulo
	    = std::numeric_limits<value_type>::is_modulo;
	static constexpr size_t digits   = Digits;
	static constexpr size_t digits10 = static_cast<size_t>(
	    Digits * (0.30102999566398119521373889472449302676818));
	static constexpr bool max_digits10   = 0;
	static constexpr bool radix          = 2;
	static constexpr type min_exponent   = type(0);
	static constexpr type min_exponent10 = type(0);
	static constexpr type max_exponent   = type(0);
	static constexpr type max_exponent10 = type(0);
	static constexpr bool traps = std::numeric_limits<value_type>::traps;
	static constexpr bool tinyness_before = false;

	static consteval type
	min() noexcept
	{
		return type(~value_type(0) << (type::digits() - 1));
	}
	static consteval type
	max() noexcept
	{
		return type(~(~value_type(0) << (type::digits() - 1)));
	}
	static consteval type
	lowest() noexcept
	{
		return min();
	}
	static consteval type
	epsilon() noexcept
	{
		return type(1);
	}
	static consteval type
	round_error() noexcept
	{
		return type(0);
	}
	static consteval type
	infinity() noexcept
	{
		return type(0);
	}
	static consteval type
	quiet_NaN() noexcept
	{
		return type(0);
	}
	static consteval type
	signaling_NaN() noexcept
	{
		return type(0);
	}
};

} // namespace std

#endif // WARTHOG_UTIL_FIXED_POINT_HPP
