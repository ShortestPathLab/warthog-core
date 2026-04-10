#ifndef WARTHOG_UTIL_SCENARIO_RUNNER_H
#define WARTHOG_UTIL_SCENARIO_RUNNER_H

// scenario_runner.h
//
// Take a scenario_manager object and be able to progress through a dynamic scenario.
//
// @author: Ryan Hechenberger
// @created: 2026-04-09
//

#include "scenario_manager.h"

namespace warthog::util
{

class scenario_runner
{
public:
	scenario_runner();
	scenario_runner(const scenario_manager* scen);
	~scenario_runner();

	/// @brief reset scenario to first command
	void
	clear();

	/// @brief reset scenario to first command
	void
	restart();

	/// @brief progress from current to count experiment away and return it
	/// @param count 
	/// @return pair of the reached query and snapshot id.
	///
	/// Will progress through commands until count queries are encounted, returning the final query.
	/// When count == 1, is exactly the next query.
	/// All patches required 
	std::pair<const experiment*, int>
	experiment_next(uint32_t count = 1);

	/// @brief goto the start of the next snapshot (SNAPSHOT command)
	/// @return snapshot id reached, or -1 if at end of commands (no more snapshots)
	///
	/// If current command is SNAPSHOT, if id != current_snapshot() then already at next snapshot,
	/// otherwise progress until next SNAPSHOT command is reached (or end of commands).
	/// Clears all patches from get_patches() and replaces with any PATCH command to next snapshot.
	int
	snapshot_next(bool clear_patch = true);

	/// @brief starting at SNAPSHOT or current PATCH, apply all patches until reaching SNAPSHOT or QUERY
	/// @param clear_patch clears get_patches()
	/// @return the number of patches, patch id are retrivable from get_patches()
	///
	/// Requires to be on SNAPSHOT or PATCH, otherwise returns 0 and does nothing.
	/// If @clear_patch is set, clears get_patches().
	/// Appends all processed PATCH commands to get_patches().
	int
	snapshot_patches(bool clear_patch = true);

	/// @brief returns the current query experiment if at query and progress to next command
	/// @return the current query experiment if command is query, otherwise nullptr
	///
	/// Requires to be on QUERY, otherwise return nullptr and do nothing.
	/// If QUERY, returns corrisponding experiment and goto next command.
	/// Does not affect get_patches().
	const experiment*
	snapshot_query();

	/// @brief progress from current to count experiment away and return it
	/// @param clear_patch clears get_patches()
	/// @return pair of the reached query and snapshot id.
	///
	/// Will progress through commands until count queries are encounted, returning the final query.
	/// When count == 1, is exactly the next query.
	/// All patches required 
	std::span<const experiment*>
	snapshot_query_all();

	bool complete() const noexcept { return command_at_ >= command_size_; }
	
	std::span<const uint32_t>
	get_patches() const noexcept
	{
		return patches_;
	}

	size_t
	mem() const noexcept
	{
		return 0;
	}

protected:
	const scenario_manager* scenario_ = nullptr;
	std::vector<uint32_t> patches_;
	std::vector<const experiment*> experiments_;
	uint32_t command_at_ = 0; ///< command at, used for dynamic scenario
	uint32_t command_size_ = 0;
	int32_t experiment_at_ = 0;
	int32_t snapshot_at_ = -1;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_SCENARIO_RUNNER_H
