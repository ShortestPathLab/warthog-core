#ifndef WARTHOG_DOMAIN_GRIDMAP_H
#define WARTHOG_DOMAIN_GRIDMAP_H

// Scenario v2 grid-based patch set.
// grid_patches primailry holds an array of bittable, handles reading from file
// and efficent memory storage.
//
// Look at utilities in manager/dynamic_gridmap.h for automatic translation to gridmap
// in a dynamic scenario.
//
// @author: Ryan Hechenberger
// @created: 2026-04-10
//

#include "grid.h"
#include <warthog/memory/bittable.h>
#include <filesystem>

namespace warthog::domain
{

class grid_patches
{
	grid_patches();
	grid_patches(std::istream& input);
	grid_patches();
	grid_patches();
};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_GRIDMAP_H
