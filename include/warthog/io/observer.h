#ifndef WARTHOG_IO_LISTENER_H
#define WARTHOG_IO_LISTENER_H

// listener/stream_listener.h
//
// The lister base class, has many invoker functions for set listen events
//
//  This class implements dummy listener with empty event handlers.
//
// @author: Ryan Hechenberger
// @created: 2025-08-06
//

#include <warthog/constants.h>

#define WARTHOG_LISTENER_HAS_FN(func_name) \
template <typename Listener, typename... Args> \
concept listener_has_##func_name = requires(Listener L, Args&&... args) \
{ \
	{ L.func_name(std::forward<Args>(args)...) }; \
};
#define WARTHOG_LISTENER_FN(func_name) \
template <size_t I = 0, typename Listeners, typename... Args> \
void listener_##func_name(Listeners& L, Args&&... args) \
{ \
	if constexpr (I < std::tuple_size_v<Listeners>) { \
		using T = std::tuple_element_t<I, Listeners>; \
		if constexpr (listener_has_##func_name <T, Args...>) { \
			/* call event first if present */ \
			if constexpr (listener_has_event <T, const char*>) std::get<I>(L).event(#func_name); \
			std::get<I>(L).func_name(std::forward<Args>(args)...); \
		} else { \
			if constexpr (listener_has_missing_event <T, const char*>) \
				std::get<I>(L).missing_event(#func_name); \
		} \
		listener_##func_name <I+1>(L, std::forward<Args>(args)...); \
	} \
}

#define WARTHOG_LISTENER_DEFINE(func_name) \
	WARTHOG_LISTENER_HAS_FN(func_name) \
	WARTHOG_LISTENER_FN(func_name)

namespace warthog::io
{

// functions used by WARTHOG_LISTENER_FN
WARTHOG_LISTENER_HAS_FN(event)
WARTHOG_LISTENER_HAS_FN(missing_event)

WARTHOG_LISTENER_DEFINE(begin_search)
WARTHOG_LISTENER_DEFINE(end_search)
WARTHOG_LISTENER_DEFINE(generate_node)
WARTHOG_LISTENER_DEFINE(expand_node)
WARTHOG_LISTENER_DEFINE(relax_node)
WARTHOG_LISTENER_DEFINE(close_node)

} // namespace warthog::io

#endif // WARTHOG_IO_LISTENER_H
