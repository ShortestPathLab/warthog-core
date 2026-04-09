#include <warthog/util/scenario_runner.h>

#include <warthog/io/log.h>
#include <warthog/search/dummy_listener.h>
#include <warthog/search/problem_instance.h>

#include <cstdlib>
#include <cstring>

namespace warthog::util
{

scenario_runner::scenario_runner() { }

scenario_runner::~scenario_runner() = default;

std::pair<const experiment*, int> scenario_runner::experiment_next(uint32_t count)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	patches_.clear(); // reset patches
	if (count == 0)
		return {nullptr, 0};
	int patch_count = 0;
	while (command_at_ < command_size_) {
		auto cmd = commands[command_at_];
		// command_at_ incremented in following fuction calls
		switch (cmd.type) {
		case scenario_command::SNAPSHOT:
		case scenario_command::PATCH:
			patch_count += snapshot_patches();
			break;
		case scenario_command::QUERY:
			if (const experiment* query = snapshot_query(); query != nullptr) {
				if (--count == 0)
					return {query, patch_count};
			}
			break;
		default:
			// should never be reached
			++command_at_;
			WARTHOG_GDEBUG("scenario_runner::experiment_next invalid command type in " WARTHOG_FILENAME_LINE);
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
	command_at_ = 0;
	experiment_at_ = -1;
	snapshot_at_ = -1;
}

int
scenario_runner::snapshot_next(bool clear_patch)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if (clear_patch)
		patches_.clear();
	while (command_at_ < commands.size()) {
		auto cmd = commands[command_at_];
		switch (cmd.type) {
		case scenario_command::SNAPSHOT:
			if (cmd.id != snapshot_at_) {
				// at new snapshot, return
				snapshot_at_ = cmd.id;
				return snapshot_at_;
			}
			// current snapshot, goto next snapshot
		[[fallthrough]];
		case scenario_command::QUERY:
			++snapshot_at_;
			break;
		case scenario_command::PATCH:
			snapshot_patches(false);
			// snapshot_patches increments snapshot_at_
			break;
		default:
			WARTHOG_GDEBUG("scenario_runner::snapshot_next invalid command type in " WARTHOG_FILENAME_LINE);
		}
	}
	return false;
}

int
scenario_runner::snapshot_patches(bool clear_patch)
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if (clear_patch)
		patches_.clear();
	int count = 0;
	// if start of snapshot, apply that snapshot patches
	if (command_at_ < commands.size() && commands[command_at_].type == scenario_command::SNAPSHOT)
		command_at_ += 1;
	// while command_at_ is PATCH, add patch to applied list
	while (command_at_ < commands.size()) {
		if (auto cmd = commands[command_at_]; cmd.bucket == scenario_command::PATCH) {
			command_at_ += 1;
			count += 1;
			patches_.push_back(cmd.id);
		}
	}
	// end at first non-PATCH command
	return count;
}

const experiment*
scenario_runner::snapshot_query()
{
	assert(scenario_ != nullptr);
	auto commands = scenario_->get_commands();
	if (command_at_ >= commands.size())
		return nullptr;
	auto cmd = commands[command_at_];
	if (cmd.type != scenario_command::QUERY)
		return nullptr;
	command_at_ += 1;
	experiment_at_ += 1;
	if (cmd.cmd.query.experiment_id >= scenario_->num_experiments() || cmd.cmd.query.experiment_id != (uint32_t)experiment_at_) {
		WARTHOG_GERROR_FMT("scenario_runner::snapshot_query invalid experiment_id {} to experiment, expected {} (max {}) in {}", cmd.cmd.query.experiment_id, experiment_at_, scenario_->num_experiments(), WARTHOG_FILENAME_LINE);
		return nullptr;
	}
	return experiments_[cmd.cmd.query.experiment_id];
}

std::span<const experiment*>
scenario_runner::snapshot_query_all()
{
	assert(scenario_ != nullptr);
	experiments_.clear();
	auto commands = scenario_->get_commands();
	auto exp = scenario_->get_experiments();
	while (command_at_ < commands.size())
	{
		auto cmd = commands[command_at_];
		if (cmd.type != scenario_command::QUERY)
			break;
		if (cmd.cmd.query.experiment_id >= exp.size() || cmd.cmd.query.experiment_id != (uint32_t)experiment_at_) {
			WARTHOG_GERROR_FMT("scenario_runner::snapshot_query_all invalid experiment_id {} to experiment, expected {} (max {}) in {}", cmd.cmd.query.experiment_id, experiment_at_, exp.size(), WARTHOG_FILENAME_LINE);
			return {};
		}
		experiments_.push_back(exp[cmd.cmd.query.experiment_id]);
		command_at_ += 1;
		experiment_at_ += 1;
	}
	return experiments_;
}

} // namespace warthog::util
