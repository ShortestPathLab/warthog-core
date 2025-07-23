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
#include <type_traits>
#include <tuple>

namespace warthog::util
{

namespace details
{

template <auto Tup>
struct for_each_tuple
{
	template <typename TemplateFunc, typename... Args>
	static constexpr void apply(TemplateFunc&& tfunc, Args&&... args)
	{
		std::apply([tfunc=std::forward<TemplateFunc>(tfunc),args=std::forward_as_tuple<Args...>(args...)] {
			apply_tfunc<t>(); }, Tup);
	}
};

// template <typename IS>
// struct conditional_apply_integer_sequence;

// template <typename IST, IST... Values>
// struct conditional_apply_integer_sequence<std::integer_sequence<IST, Values...>>
// {
// 	template <typename TemplateFunc, typename... Args>
// 	static constexpr decltype(auto) apply(IST v, TemplateFunc&& tfunc)
// 	{
// 		if constexpr (sizeof...(Values) != 0) {
// 			using ret = std::invoke_result_t<decltype(TemplateFunc), std::integral_constant<IST, Values>()>;
// 			return apply_<ret, Values...>(v, std::forward<TemplateFunc>(tfunc));
// 		}
// 	}
// 	template <typename Ret, IST V0, IST... VN, typename TemplateFunc>
// 	static constexpr void apply_(IST v, TemplateFunc&& tfunc)
// 	{
// 		if (v == V0) {
// 			return std::invoke_r<Ret>(std::forward<TemplateFunc>(tfunc), 
// 	}
// };

} // namespace details

// template <typename IS, typename TemplateFunc>
// void for_each_integer_sequence(TemplateFunc&& tfunc);

template <auto Tup, typename TemplateFunc, typename... Args>
void for_each_tuple(TemplateFunc&& tfunc, Args... args)
{
	details::for_each_tuple<Tup>::apply(std::forward<TemplateFunc>(tfunc), std::forward<Args>(args)...);
}

} // namespace warthog::util

#endif // WARTHOG_UTIL_CAST_H
