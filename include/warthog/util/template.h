#ifndef WARTHOG_UTIL_TEMPLATE_H
#define WARTHOG_UTIL_TEMPLATE_H

// template.h
//
// Utility file for template metaprogramming.
//
// @author: Ryan Hechenberger
// @created: 2025-06-27
//

#include <utility>

namespace warthog::util
{

// implementation details, user function after namespace
namespace details
{

template<typename IS>
struct for_each_integer_sequence;

template<typename IST, IST... Values>
struct for_each_integer_sequence<std::integer_sequence<IST, Values...>>
{
	template<typename TemplateFunc>
	static constexpr void
	apply(TemplateFunc&& tfunc)
	{
		(..., tfunc(std::integral_constant<IST, Values>()));
	}

	template<typename Ret, typename TemplateFunc>
	static constexpr void
	apply_if(IST value, TemplateFunc&& tfunc)
	{
		if constexpr(std::is_void_v<Ret>)
		{
			apply_if_aux_<Ret, Values...>(
			    value, std::forward<TemplateFunc>(tfunc));
		}
	}
	template<typename Ret, IST Arg0, IST... Args, typename TemplateFunc>
	static constexpr Ret
	apply_if_aux_(IST value, TemplateFunc&& tfunc)
	{
		if(Arg0 == value)
		{
			return static_cast<Ret>(
			    tfunc(std::integral_constant<IST, Arg0>()));
		}
		else
		{
			return apply_if_aux_<Ret, Args...>(
			    value, std::forward<TemplateFunc>(tfunc));
		}
	}
	template<typename Ret, typename TemplateFunc>
	static constexpr Ret
	apply_if_aux_(IST value, TemplateFunc&& tfunc)
	{
		return Ret();
	}
};

} // namespace details

// template <typename IS, typename TemplateFunc>
// void for_each_integer_sequence(TemplateFunc&& tfunc);

/// @brief takes an std::integer_sequence and pass each value to tfunc as a
/// std::integral_constant
/// @tparam IST loop over this std::integer_sequence
/// @param tfunc a function that takes an std::integral_constant of value from
/// IST
template<typename IST, typename TemplateFunc>
void
for_each_integer_sequence(TemplateFunc&& tfunc)
{
	details::for_each_integer_sequence<IST>::apply(
	    std::forward<TemplateFunc>(tfunc));
}

/// @brief takes an std::integer_sequence and pass tfunc a
/// std::integral_constant that matches value
/// @tparam IST loop over this std::integer_sequence
/// @param value a value in IST to call tfunc on
/// @param tfunc a function that takes an std::integral_constant of value
template<typename IST, typename TemplateFunc>
void
choose_integer_sequence(auto value, TemplateFunc&& tfunc)
{
	details::for_each_integer_sequence<IST>::template apply_if<void>(
	    value, std::forward<TemplateFunc>(tfunc));
}
/// @brief takes an std::integer_sequence and pass tfunc a
/// std::integral_constant and returns
/// @tparam IST loop over this std::integer_sequence
/// @param value a value in IST to call tfunc on
/// @param tfunc a function that takes an std::integral_constant of value
/// @return returns value from tfunc call, or Ret{} if value does not match any
/// IST
template<typename Ret, typename IST, typename TemplateFunc>
Ret
choose_integer_sequence(auto value, TemplateFunc&& tfunc)
{
	return details::for_each_integer_sequence<IST>::template apply_if<Ret>(
	    value, std::forward<TemplateFunc>(tfunc));
}

template<typename T, typename T2>
concept same_as_rmref
    = std::same_as<std::remove_reference_t<T>, std::remove_reference_t<T2>>;
template<typename T, typename T2>
concept same_as_rmcvref
    = std::same_as<std::remove_cvref_t<T>, std::remove_cvref_t<T2>>;

namespace details
{
template<typename T>
struct TypeTemplate : std::false_type
{ };
template<template<typename...> typename T, typename... Ts>
struct TypeTemplate<T<Ts...>> : std::true_type
{ };
template<typename T>
struct TypeTemplateSize : std::integral_constant<size_t, 0>
{ };
template<template<typename...> typename T, typename... Ts>
struct TypeTemplateSize<T<Ts...>>
    : std::integral_constant<size_t, sizeof...(Ts)>
{ };
template<typename T>
struct TupleType : std::false_type
{ };
template<typename... Ts>
struct TupleType<std::tuple<Ts...>> : std::true_type
{ };
} // namespace details

//
// BEGIN inxlib https://github.com/heavenfall/inxlib
//

template<typename T>
concept Plain
    = !std::is_void_v<T> && !std::is_reference_v<T> && !std::is_volatile_v<T>;
template<typename T>
concept ConstPlain = Plain<T> && std::is_const_v<T>;
template<typename T>
concept NonConstPlain = Plain<T> && !std::is_const_v<T>;

template<typename T>
concept TypeTemplate = details::TypeTemplate<T>::value;
template<typename T>
constexpr size_t TypeTemplateSize = details::TypeTemplateSize<T>::value;
template<typename T>
concept Tuple = details::TupleType<T>::value;

namespace details
{

template<typename From, typename To>
struct copy_ref_
{
	using type = To;
};
template<typename From, typename To>
    requires(std::is_lvalue_reference_v<From>)
struct copy_ref_<From, To>
{
	using type = std::add_lvalue_reference_t<To>;
};
template<typename From, typename To>
    requires(std::is_rvalue_reference_v<From>)
struct copy_ref_<From, To>
{
	using type = std::add_rvalue_reference_t<To>;
};

template<typename From, typename To>
struct copy_const_
{
	using type = To;
};
template<typename From, typename To>
    requires(std::is_const_v<From>)
struct copy_const_<From, To>
{
	using type = std::add_const_t<To>;
};

template<typename From, typename To>
struct copy_volatile_
{
	using type = To;
};
template<typename From, typename To>
    requires(std::is_volatile_v<From>)
struct copy_volatile_<From, To>
{
	using type = std::add_volatile_t<To>;
};

} // namespace details

/// Copy cvref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=const double&
template<typename From, typename To>
using copy_cvref = typename details::copy_ref_<
    From,
    typename details::copy_volatile_<
        std::remove_reference_t<From>,
        typename details::copy_const_<
            std::remove_reference_t<From>,
            std::remove_cvref_t<To>>::type>::type>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=double&
template<typename From, typename To>
using copy_ref =
    typename details::copy_ref_<From, std::remove_cvref_t<To>>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double&, Result=const double
template<typename From, typename To>
using copy_const = typename details::copy_const_<
    std::remove_reference_t<From>, std::remove_cvref_t<To>>::type;

/// Copy ref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double&, Result=double
template<typename From, typename To>
using copy_volatile = typename details::copy_volatile_<
    std::remove_reference_t<From>, std::remove_cvref_t<To>>::type;

/// Copy cvref (if any) of type From to type To, overriding unto To.
/// e.g. From=const int&, To=volatile double, Result=const double&
template<typename From, typename To>
using copy_cv = typename details::copy_volatile_<
    std::remove_reference_t<From>,
    typename details::copy_const_<
        std::remove_reference_t<From>, std::remove_cvref_t<To>>::type>::type;

//
// END inxlib
//

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
