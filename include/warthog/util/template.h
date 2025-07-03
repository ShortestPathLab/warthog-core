#ifndef WARTHOG_UTIL_TEMPLATE_H
#define WARTHOG_UTIL_TEMPLATE_H

// cast.h
//
// Utility file for memory value conversion.
// Include support for byte swapping over different compilers.
//
// @author: Ryan Hechenberger
// @created: 2025-06-27
//

#include <utility>

namespace warthog::util
{

namespace details
{

template <typename IS>
struct for_each_integer_sequence;

template <typename IST, IST... Values>
struct for_each_integer_sequence<std::integer_sequence<IST, Values...>>
{
	template <typename TemplateFunc>
	static constexpr void apply(TemplateFunc&& tfunc)
	{
		(tfunc(std::integral_constant<IST, Values>()), ...);
	}
};

} // namespace details

// template <typename IS, typename TemplateFunc>
// void for_each_integer_sequence(TemplateFunc&& tfunc);

template <typename IST, typename TemplateFunc>
void for_each_integer_sequence(TemplateFunc&& tfunc)
{
	details::for_each_integer_sequence<IST>::apply(std::forward<TemplateFunc>(tfunc));
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
