#ifndef WARTHOG_SCENARIO_SCENARIO_RUNNER_H
#define WARTHOG_SCENARIO_SCENARIO_RUNNER_H

/// @file scenario_runner.h
///
/// Take a scenario_manager object and be able to progress through a dynamic
/// scenario.
///
/// @author: Ryan Hechenberger
/// @created: 2026-04-09
///

#include "grid_patch_set.h"
#include "scenario_manager.h"
#include <warthog/domain/gridmap.h>

namespace warthog::scenario
{

struct patch_loc
{
	uint32_t patch_id;
	uint16_t topleft_x;
	uint16_t topleft_y;
};

class scenario_runner
{
public:
	scenario_runner();
	scenario_runner(const scenario_manager* scen);
	~scenario_runner();

	/// @brief clear scenario and reset the class
	void
	clear();

	/// @brief reset scenario to first command
	void
	restart();

	/// @brief get the current scenario
	const scenario_manager* get_scenario() const noexcept
	{
		return scenario_;
	}

	/// @brief clear() and set the current scenario
	void set_scenario(const scenario_manager* scen) noexcept
	{
		clear();
		scenario_ = scen;
	}

	/// @brief progress from current to count experiment away and return it
	/// @param count get number of experiments away
	/// @param progress progress past last count experiment if true
	/// @return pair of the reached inst and number of patches encounted
	///
	/// Will progress through commands until count queries are encounted,
	/// returning the final inst. When count == 1, is exactly the next inst.
	/// All patches required
	std::pair<const experiment*, int>
	experiment_next(uint32_t count = 1, bool progress = true);

	/// @brief goto the start of the next snapshot (SNAPSHOT command)
	/// @param clear_patch clears get_patches()
	/// @return snapshot id reached, or -1 if at end of commands (no more
	/// snapshots)
	///
	/// If current command is SNAPSHOT, if id != current_snapshot() then
	/// already at next snapshot, otherwise progress until next SNAPSHOT
	/// command is reached (or end of commands). Clears all patches from
	/// get_patches() and replaces with any PATCH command to next snapshot.
	int
	snapshot_next(bool clear_patch = true);

	/// @brief starting at SNAPSHOT or current PATCH, apply all patches until
	/// reaching SNAPSHOT or INST
	/// @param clear_patch clears get_patches()
	/// @return the number of patches, patch id are retrivable from
	/// get_patches()
	///
	/// Requires to be on SNAPSHOT or PATCH, otherwise returns 0 and does
	/// nothing. If @clear_patch is set, clears get_patches(). Appends all
	/// processed PATCH commands to get_patches().
	int
	snapshot_patches(bool clear_patch = true);

	/// @brief returns the current inst experiment if at inst and progress to
	/// next command (if progress)
	/// @param progress progress to next command if true, false stay on inst
	/// @return the current inst experiment if command is inst, otherwise
	/// nullptr
	///
	/// Requires to be on INST, otherwise return nullptr and do nothing.
	/// If INST, returns corrisponding experiment and goto next command.
	/// Does not affect get_patches().
	const experiment*
	snapshot_inst(bool progress = true);

	/// @brief progress from current to count experiment away and return it
	/// @param clear_patch clears get_patches()
	/// @return pair of the reached inst and snapshot id.
	///
	/// Will progress through commands until count queries are encounted,
	/// returning the final inst. When count == 1, is exactly the next inst.
	/// All patches required
	std::span<const experiment*>
	snapshot_inst_all();

	uint32_t
	get_command_at() const noexcept
	{
		return command_at_;
	}

	int32_t
	get_experiment_at() const noexcept
	{
		return experiment_at_;
	}

	int32_t
	get_snapshot_at() const noexcept
	{
		return snapshot_at_;
	}

	bool
	complete() const noexcept
	{
		return command_at_ >= scenario_->get_commands().size();
	}

	std::span<const patch_loc>
	get_patches() const noexcept
	{
		return patches_;
	}

	size_t
	mem() const noexcept
	{
		return 0;
	}

	/// @brief setup grid and contain snapshot 0
	/// @param grid the gridmap to setup
	/// @param patch_set the patch set to
	/// @param setup_grid re-create the grid to match scenario size
	/// @return true if operation is successful
	bool
	gridmap_init(
	    domain::gridmap& grid, const grid_patch_set& patch_set,
	    bool setup_grid = true);

	/// @brief apply patches from patch_set in order by get_patches()
	/// @param grid grid to apply to
	/// @param patch_set patch_set to pull patch data from
	/// @return >=0 successful patches applied, <0 failed to apply negative id
	/// (from 1)
	int
	gridmap_apply_patches(
	    domain::gridmap& grid, const grid_patch_set& patch_set);

	/// @brief apply a single patch to a gridmap
	/// @param grid grid to apply to
	/// @param patch patch bittable to copy
	/// @param padded_x the padded topleft on grid
	/// @param padded_y the padded topleft on grid
	/// @return true if apply is successful, false otherwise
	bool
	girdmap_apply_patch(
	    domain::gridmap& grid, domain::gridmap::bittable patch,
	    uint32_t padded_x, uint32_t padded_y);

protected:
	const scenario_manager* scenario_ = nullptr;
	std::vector<patch_loc> patches_;
	std::vector<const experiment*> experiments_;
	uint32_t command_at_   = 0; ///< command at, used for dynamic scenario
	int32_t experiment_at_ = -1;
	int32_t snapshot_at_   = -1;
};

} // namespace warthog::scenario

#endif // WARTHOG_SCENARIO_SCENARIO_RUNNER_H
