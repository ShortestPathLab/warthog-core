#ifndef WARTHOG_IO_OBSERVER_H
#define WARTHOG_IO_OBSERVER_H

// io/observer.h
//
// The observer pattern defines methods to tightly-bind user provided observers to certain event patterns.
// Provide a tuple of observers, where some event is triggered will notifiy all observers with function of event name that is callable.
//
// These function names must be registered before use, common ones registered here.
//
// To register a new function name, use WARTHOG_OBSERVER_DEFINE([function]).
// Invoke event with observer_[function](listeners, args...) where listeners are tuple of observers.
// This will run through each element in tuple (i) and call i.[function](args...) if able.
// If i.event([function],args...) is a valid callable, calls this function first, also tries i.event([function]).
//
// @author: Ryan Hechenberger
// @created: 2025-08-06
//

#include <warthog/constants.h>

/// @brief Creates a concept observer_has_[func_name] that checks of function Listener.func_name(args...) is callable.
#define WARTHOG_OBSERVER_DEFINE_HAS(func_name) \
template <typename Listener, typename... Args> \
concept observer_has_##func_name = requires(Listener L, Args&&... args) \
{ \
	{ L.func_name(std::forward<Args>(args)...) }; \
};
/// @brief Creates function observer_[func_name] that calls Listener.[func_name] is callable.
///        WARTHOG_OBSERVER_DEFINE_HAS([func_name]) must be defined.
///
/// The created function observer_[func_name] loops through all listeners in tuple L.
/// For each it will try to call:
/// - event([func_name], args...) if able otherwise event([func_name]) if able
/// - [func_name](args...) if able
#define WARTHOG_OBSERVER_DEFINE_CALL(func_name) \
template <size_t I = 0, typename Listeners, typename... Args> \
void observer_##func_name(Listeners& L, Args&&... args) \
{ \
	if constexpr (I < std::tuple_size_v<Listeners>) { \
		using T = std::tuple_element_t<I, Listeners>; \
		constexpr bool has_func = observer_has_##func_name <T, Args...>; \
		if constexpr (observer_has_event<T, const char*, Args...>) { std::get<I>(L).event( #func_name , std::forward<Args>(args)... ); } \
		else if constexpr (observer_has_event<T, const char*>) { std::get<I>(L).event( #func_name ); } \
		if constexpr (has_func) { \
			std::get<I>(L).func_name(std::forward<Args>(args)...); \
		} \
		observer_##func_name <I+1>(L, std::forward<Args>(args)...); \
	} \
}

/// @brief Defines a observer_[func_name] function and observer_has_[func_name] concept.
#define WARTHOG_OBSERVER_DEFINE(func_name) \
	WARTHOG_OBSERVER_DEFINE_HAS(func_name) \
	WARTHOG_OBSERVER_DEFINE_CALL(func_name)

namespace warthog::io
{

// functions used by WARTHOG_LISTENER_FN
WARTHOG_OBSERVER_DEFINE_HAS(event)

WARTHOG_OBSERVER_DEFINE(begin_search)
WARTHOG_OBSERVER_DEFINE(end_search)
WARTHOG_OBSERVER_DEFINE(generate_node)
WARTHOG_OBSERVER_DEFINE(expand_node)
WARTHOG_OBSERVER_DEFINE(relax_node)
WARTHOG_OBSERVER_DEFINE(close_node)

} // namespace warthog::io

#endif // WARTHOG_IO_OBSERVER_H
