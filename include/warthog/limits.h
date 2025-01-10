#ifndef WARTHOG_LIMITS_H
#define WARTHOG_LIMITS_H

#include "constants.h"

namespace warthog
{

// specific hard limits, must fit on stack
constexpr size_t GRID_DIMENSION_MAX = 10'000;

} // namespace warthog

#endif // WARTHOG_LIMITS_H
