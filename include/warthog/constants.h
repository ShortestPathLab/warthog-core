#ifndef WARTHOG_CONSTANTS_H
#define WARTHOG_CONSTANTS_H

// #include <warthog/constants.h>
//
// @author: dharabor
// @created: 01/08/2012
//

#include <bit>
#include <cfloat>
#include <climits>
#include <cmath>
#include <stdint.h>

namespace warthog
{

using sn_id_t = uint64_t; // address space for state identifiers
constexpr sn_id_t SN_ID_MAX = UINT64_MAX;
constexpr sn_id_t NO_PARENT = SN_ID_MAX;

// each node in a weighted grid map uses sizeof(dbword) memory.
// in a uniform-cost grid map each dbword is a contiguous set
// of nodes s.t. every bit represents a node.
using dbword = uint8_t;

// gridmap constants
constexpr uint32_t DBWORD_BITS = sizeof(warthog::dbword) * 8;
constexpr uint32_t DBWORD_BITS_MASK = (warthog::DBWORD_BITS - 1);
constexpr uint32_t LOG2_DBWORD_BITS = std::popcount(DBWORD_BITS_MASK);

// search and sort constants
constexpr double DBL_ONE = 1.0;
constexpr double DBL_TWO = 2.0;
constexpr double DBL_ROOT_TWO
    = 1.414213562373095048801688724209698078569671875;
constexpr double DBL_ONE_OVER_TWO = 0.5;
constexpr double DBL_ONE_OVER_ROOT_TWO = 1.0 / DBL_ROOT_TWO; // 0.707106781f;
constexpr double DBL_ROOT_TWO_OVER_FOUR = DBL_ROOT_TWO * 0.25;
constexpr int32_t ONE = 100000;

constexpr uint32_t INF32
    = UINT32_MAX; // indicates uninitialised or undefined values
constexpr uint64_t INFTY
    = UINT64_MAX; // indicates uninitialised or undefined values

using cost_t = double;
constexpr cost_t COST_MAX = DBL_MAX;
constexpr cost_t COST_MIN = DBL_MIN;

// hashing constants
constexpr uint32_t FNV32_offset_basis = 2166136261;
constexpr uint32_t FNV32_prime = 16777619;

constexpr double DIMACS_RATIO = 1e6;

}

#endif // WARTHOG_CONSTANTS_H
