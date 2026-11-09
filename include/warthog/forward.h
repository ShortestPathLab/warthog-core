#ifndef WARTHOG_FORWARD_H
#define WARTHOG_FORWARD_H

// ./memory/forward.h
//
// Forward declarations
//
// @author: dharabor
// @created: 2018-11-07
//

#include <cstdint>
#include <cstddef>

namespace warthog
{

// using clause
namespace forward_types
{

using std::size_t;
using std::ptrdiff_t;
using std::max_align_t;

using std::int8_t;
using std::int16_t;
using std::int32_t;
using std::int64_t;
using std::intmax_t;
using std::intptr_t;

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;
using std::uint64_t;
using std::uintmax_t;
using std::uintptr_t;

}

using namespace forward_types;

namespace domain
{
class gridmap;
template<class CELL>
class labelled_gridmap;

}

} // namespace warthog

#endif // WARTHOG_FORWARD_H
