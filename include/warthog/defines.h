#ifndef WARTHOG_DEFINES_H
#define WARTHOG_DEFINES_H

#if __has_include(<warthog/config.h>)
#include <warthog/config.h>

#ifdef WARTHOG_INT128
#if defined(__GNUC__) && defined(__SIZEOF_INT128__)
#define WARTHOG_INT128_ENABLED
#endif
#endif // WARTHOG_INT128

#endif // <warthog/config.h>

#endif // WARTHOG_DEFINES_H
