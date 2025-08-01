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
			apply_if_aux_<void, Values...>(std::forward<TemplateFunc>(tfunc));
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
			return apply_if_aux_<void, Values...>(
			    std::forward<TemplateFunc>(tfunc));
		}
	}
	template<typename Ret, typename TemplateFunc>
	static constexpr Ret
	apply_if_aux_(IST value, TemplateFunc&& tfunc)
	{
		return Ret{};
	}
};

} // namespace details

// template <typename IS, typename TemplateFunc>
// void for_each_integer_sequence(TemplateFunc&& tfunc);

/// @brief takes an std::integer_sequence and pass each value to tfunc as a std::integral_constant
/// @tparam IST loop over this std::integer_sequence
/// @param tfunc a function that takes an std::integral_constant of value from IST
template<typename IST, typename TemplateFunc>
void
for_each_integer_sequence(TemplateFunc&& tfunc)
{
	details::for_each_integer_sequence<IST>::apply(
	    std::forward<TemplateFunc>(tfunc));
}

/// @brief takes an std::integer_sequence and pass tfunc a std::integral_constant that matches value
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
/// @brief takes an std::integer_sequence and pass tfunc a std::integral_constant and returns
/// @tparam IST loop over this std::integer_sequence
/// @param value a value in IST to call tfunc on
/// @param tfunc a function that takes an std::integral_constant of value
/// @return returns value from tfunc call, or Ret{} if value does not match any IST
template<typename Ret, typename IST, typename TemplateFunc>
Ret
choose_integer_sequence(auto value, TemplateFunc&& tfunc)
{
	return details::for_each_integer_sequence<IST>::template apply_if<Ret>(
	    value, std::forward<TemplateFunc>(tfunc));
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
