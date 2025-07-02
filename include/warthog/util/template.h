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

template <typename IS, typename TemplateFunc>
void for_each_integer_sequence(Func&& tfunc);

template <typename TemplateFunc, typename IST, IST... Values>
void for_each_integer_sequence<std::integer_sequence<IST, Values...>, TemplateFunc>(Func&& tfunc)
{
	(tfunc<Values>(), ...);
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
