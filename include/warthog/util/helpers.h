#ifndef WARTHOG_UTIL_HELPERS_H
#define WARTHOG_UTIL_HELPERS_H

// helpers.h
//
// Helper functions that don't fit anywhere else.
//
// @author: dharabor
// @created: 21/08/2012
//

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <warthog/constants.h>

namespace warthog::util
{

// convert a one-dimensional grid index into x/y coordinates
inline void
index_to_xy(uint32_t grid_id, uint32_t mapwidth, int32_t& x, int32_t& y)
{
	y = (int32_t)(grid_id / mapwidth);
	x = (int32_t)(grid_id % mapwidth);
}

// convert from one address space to another
// inline uint32_t
// convert_id_sn_to_xy(sn_id_t in) { return (uint32_t)in; }

// sometimes the nodes of the search graph need to be labeled with
// integer data (e.g. during a partitioning of the graph)
// this file loads up such integer labels from a file. It assumes each
// line of the file contains a single integer.
// Comments lines can exist in the file; these need to begin with one
// of the following characters: #, %, c
bool
load_integer_labels(const char* filename, std::vector<uint32_t>& labels);

bool
load_integer_labels_dimacs(
    const char* filename, std::vector<uint32_t>& labels);

// re-maps @param vec s.t. for each x and i
// v[i] = x becomes v[x] = i
void
value_index_swap_array(std::vector<uint32_t>& vec);

} // namespace warthog::util

#endif // WARTHOG_UTIL_HELPERS_H
