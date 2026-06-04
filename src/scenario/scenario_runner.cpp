#include <warthog/scenario/scenario_runner.h>

#include <warthog/io/log.h>
#include <warthog/search/dummy_listener.h>
#include <warthog/search/problem_instance.h>

#include <cstdlib>
#include <cstring>

namespace warthog::scenario
{

scenario_runner::scenario_runner() = default;

scenario_runner::scenario_runner(const scenario_manager* scen)
    : scenario_(scen)
{ }

scenario_runner::~scenario_runner() = default;

std::pair<const experiment*, int>
scenario_runner::experiment_next(uint32_t count)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	patches_.clear(); // reset patches
	if(count == 0) return {nullptr, 0};
	int patch_count = 0;
	while(command_at_ < commands.size())
	{
		auto cmd = commands[command_at_];
		// command_at_ incremented in following fuction calls
		switch(cmd.type)
		{
		case scenario_command::SNAPSHOT:
		case scenario_command::PATCH:
			patch_count += snapshot_patches();
			break;
		case scenario_command::INST:
			if(const experiment* inst = snapshot_inst(); inst != nullptr)
			{
				if(--count == 0) return {inst, patch_count};
			}
			break;
		default:
			// should never be reached
			++command_at_;
			WARTHOG_GDEBUG("scenario_runner::experiment_next invalid command "
			               "type in " WARTHOG_FILENAME_LINE);
		}
		// exits loop
	}
	// no more experiments
	return {nullptr, patch_count};
}

void
scenario_runner::clear()
{
	restart();
	scenario_ = nullptr;
}

void
scenario_runner::restart()
{
	patches_.clear();
	experiments_.clear();
	command_at_    = 0;
	experiment_at_ = -1;
	snapshot_at_   = -1;
}

int
scenario_runner::snapshot_next(bool clear_patch)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if(clear_patch) patches_.clear();
	while(command_at_ < commands.size())
	{
		auto cmd = commands[command_at_];
		switch(cmd.type)
		{
		case scenario_command::SNAPSHOT:
			if(cmd.id != snapshot_at_)
			{
				// at new snapshot, return
				snapshot_at_ = cmd.id;
				return snapshot_at_;
			}
			// current snapshot, goto next snapshot
			[[fallthrough]];
		case scenario_command::INST:
			++experiment_at_;
			break;
		case scenario_command::PATCH:
			snapshot_patches(false);
			// snapshot_patches increments snapshot_at_
			break;
		default:
			WARTHOG_GDEBUG("scenario_runner::snapshot_next invalid command "
			               "type in " WARTHOG_FILENAME_LINE);
		}
	}
	return false;
}

int
scenario_runner::snapshot_patches(bool clear_patch)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if(clear_patch) patches_.clear();
	int count = 0;
	// if start of snapshot, apply that snapshot patches
	if(command_at_ < commands.size()
	   && commands[command_at_].type == scenario_command::SNAPSHOT)
	{
		snapshot_at_ = commands[command_at_].id;
		command_at_ += 1;
	}
	// while command_at_ is PATCH, add patch to applied list
	while(command_at_ < commands.size())
	{
		if(auto cmd = commands[command_at_];
		   cmd.type == scenario_command::PATCH)
		{
			command_at_ += 1;
			count       += 1;
			patches_.push_back(
			    {cmd.id, cmd.cmd.patch.topleft_x, cmd.cmd.patch.topleft_y});
		}
		else { break; }
	}
	// end at first non-PATCH command
	return count;
}

const experiment*
scenario_runner::snapshot_inst()
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if(command_at_ >= commands.size()) return nullptr;
	auto cmd = commands[command_at_];
	if(cmd.type != scenario_command::INST) return nullptr;
	command_at_    += 1;
	experiment_at_ += 1;
	if(cmd.cmd.inst.experiment_id >= scenario_->num_experiments()
	   || cmd.cmd.inst.experiment_id != (uint32_t)experiment_at_)
	{
		WARTHOG_GERROR_FMT(
		    "scenario_runner::snapshot_inst invalid experiment_id {} to "
		    "experiment, expected {} (max {}) in {}",
		    cmd.cmd.inst.experiment_id, experiment_at_,
		    scenario_->num_experiments(), WARTHOG_FILENAME_LINE);
		return nullptr;
	}
	return scenario_->get_experiment(cmd.cmd.inst.experiment_id);
}

std::span<const experiment*>
scenario_runner::snapshot_inst_all()
{
	assert(scenario_ != nullptr);
	experiments_.clear();
	auto commands = scenario_->get_commands();
	auto exp      = scenario_->get_experiments();
	while(command_at_ < commands.size())
	{
		auto cmd = commands[command_at_];
		if(cmd.type != scenario_command::INST) break;
		if(cmd.cmd.inst.experiment_id >= exp.size()
		   || cmd.cmd.inst.experiment_id != (uint32_t)experiment_at_)
		{
			WARTHOG_GERROR_FMT(
			    "scenario_runner::snapshot_inst_all invalid experiment_id {} "
			    "to experiment, expected {} (max {}) in {}",
			    cmd.cmd.inst.experiment_id, experiment_at_, exp.size(),
			    WARTHOG_FILENAME_LINE);
			return {};
		}
		experiments_.push_back(exp[cmd.cmd.inst.experiment_id]);
		command_at_    += 1;
		experiment_at_ += 1;
	}
	return experiments_;
}

bool
scenario_runner::gridmap_init(
    domain::gridmap& grid, const grid_patch_set& patch_set, bool setup_grid)
{
	if(setup_grid)
	{
		grid.setup(
		    scenario_->get_scenario_height(), scenario_->get_scenario_width());
		// grid is 0 (non-traversable), make map area traversable but keep
		// padding non-traversable
		for(uint32_t y = 0, ye = grid.header_height(),
		             xe = grid.header_width();
		    y < ye; ++y)
		{
			pad_id row_id = grid.to_padded_id_from_unpadded(0, y);
			for(uint32_t x = 0; x < xe; ++x, ++row_id.id)
			{
				grid.set_label(row_id, true);
			}
		}
	}
	restart();
	snapshot_patches();
	return gridmap_apply_patches(grid, patch_set) >= 0;
}
int
scenario_runner::gridmap_apply_patches(
    domain::gridmap& grid, const grid_patch_set& patch_set)
{
	int count = 0;
	for(auto& P : patches_)
	{
		++count;
		if(P.patch_id >= patch_set.size()) return -count;
		uint32_t x, y;
		grid.to_padded_xy_from_unpadded(P.topleft_x, P.topleft_y, x, y);
		if(!girdmap_apply_patch(grid, patch_set.get_patch(P.patch_id), x, y))
		{
			return -count;
		}
	}
	return count;
}
bool
scenario_runner::girdmap_apply_patch(
    domain::gridmap& grid, domain::gridmap::bittable patch, uint32_t padded_x,
    uint32_t padded_y)
{
	if((uint64_t)padded_x + patch.width() >= (uint64_t)grid.width()
	   || (uint64_t)padded_y + patch.height() >= (uint64_t)grid.height())
		return false;

	// apply patch
	patch.copy(
	    grid, grid.to_padded_id_from_padded(padded_x, padded_y),
	    pad_id::zero(), patch.width(), patch.height());
	return true;
}

} // namespace warthog::scenario
