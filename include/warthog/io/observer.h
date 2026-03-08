#ifndef WARTHOG_IO_OBSERVER_H
#define WARTHOG_IO_OBSERVER_H

// io/observer.h
//
// Defines use of an observer pattern, in which observer object is registered
// to an observable, and the observable will trigger events to all relevant
// observers.
// The observer is passed to an observable as a list of tuples,
// and when triggering an event will notify all observers by function call of
// the event name that is callable to the observer.
// Observer are either stored as value in the tuple or pointer to an observer.
//
// These function names must be registered before use, common ones registered
// here.
//
// To register a new function name, use WARTHOG_OBSERVER_DEFINE([function]).
// Invoke event with observer_[function](listeners, args...) where listeners
// are tuple of observers. This will run through each element in tuple (i) and
// call i.[function](args...) if able. If i.event([function],args...) is a
// valid callable, calls this function first, also tries i.event([function]).
//
// @author: Ryan Hechenberger
// @created: 2025-08-06
//

#include <warthog/constants.h>

/// @brief Creates a concept observer_has_[func_name] that checks of function
/// Listener.func_name(args...) is callable.
#define WARTHOG_OBSERVER_DEFINE_HAS(func_name)                                \
	template<typename Listener, typename... Args>                             \
	concept observer_has_##func_name = requires(Listener L, Args&&... args) { \
		{ L.func_name(std::forward<Args>(args)...) };                         \
	};
/// @brief Creates function observer_[func_name] that triggers an event to the observer.
/// 
/// Creates function observer_[func_name], which will trigger relevant observer function
/// to all observers in tuple. Requires WARTHOG_OBSERVER_DEFINE_HAS([func_name]).
/// Operates as observer_[func_name](observers, args...), where observers is a tuple of observers.
/// If an observer is a value, is owned by the owner of the tuple.
/// If an observer is a pointer, is not owned by the tuple, and will only trigger events to
/// an observer that is not null.
///
/// Events are triggered for observers in order of the tuple, and events triggered
/// in order if exists in observer:
/// - event([func_name], args...) if exists otherwise event([func_name]) if exists
/// - [func_name](args...) if exists
#define WARTHOG_OBSERVER_DEFINE_CALL(func_name)                               \
	template<size_t I = 0, typename Listeners, typename... Args>              \
	void observer_##func_name(Listeners& L, Args&&... args)                   \
	{                                                                         \
		if constexpr(I < std::tuple_size_v<Listeners>)                        \
		{                                                                     \
			using T                 = std::tuple_element_t<I, Listeners>;     \
			using Tb                = std::remove_pointer_t<T>;                \
			constexpr bool has_func = observer_has_##func_name<Tb, Args...>;   \
			if constexpr(::warthog::io::observer_has_event<Tb, const char*, Args...>)        \
			{\
				if constexpr (std::is_pointer_v<T>) {\
					if (auto* p = std::get<I>(L); p != nullptr)\
						p->event(#func_name, std::forward<Args>(args)...);\
				} else {                                                                 \
				std::get<I>(L).event(                                         \
				    #func_name, std::forward<Args>(args)...); \
				                }                \
			}                                                                 \
			else if constexpr(::warthog::io::observer_has_event<Tb, const char*>)            \
			{                                                                 \
				if constexpr (std::is_pointer_v<T>) {\
					if (auto* p = std::get<I>(L); p != nullptr)\
						p->event(#func_name);\
				} else {                                                                 \
				std::get<I>(L).event(#func_name); \
				                }                \
			}                                                                 \
			if constexpr(has_func)                                            \
			{                        \
				if constexpr (std::is_pointer_v<T>) { \
					if (auto* p = std::get<I>(L); p != nullptr) \
						p->func_name(std::forward<Args>(args)...);   \
				} else { \
					std::get<I>(L).func_name(std::forward<Args>(args)...);        \
				} \
			}                                                                 \
			observer_##func_name<I + 1>(L, std::forward<Args>(args)...);      \
		}                                                                     \
	}

/// @brief Defines a observer_[func_name] function and observer_has_[func_name]
/// concept.
#define WARTHOG_OBSERVER_DEFINE(func_name)                                    \
	WARTHOG_OBSERVER_DEFINE_HAS(func_name)                                    \
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
