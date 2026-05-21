#ifndef WARTHOG_IO_SCENARIO_H
#define WARTHOG_IO_SCENARIO_H

/// @file scenario.h
///
/// Read/write utilities for scenario files.
///
///	Supported formats for read:
///	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
///		  (fields: bucket,map,mapwidth,mapheight,sx,sy,gx,gy,distance)
///	    - DIMACS format (as at the 9th DIMACS Implementation Challenge)
///	      (fields: q [source-id] [target-id])
///     - Dynamic format (tbd link)
///
///	Supported formats for generate/write:
///	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
///      - Dynamic format (tbd link)
///
/// @author: dharabor & Ryan Hechenberger
/// @created: 2025-12-04
///

#include "serialize_base.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <memory_resource>
#include <span>
#include <sstream>
#include <vector>

namespace warthog::io
{

/// @brief scenario instance struct, reusable with fields set by
/// scenario_serialize
struct scenario_instance
{
	int64_t bucket;
	std::string_view map;
	int32_t width;
	int32_t height;
	double start_x;
	double start_y;
	double goal_x;
	double goal_y;
	std::span<double> cost;
	void* extra_data; ///< holds extra data that a user may require (not used
	                  ///< by default)

	void
	reset()
	{
		(*this) = {};
	}
};

struct scenario_patch
{
	int64_t bucket;
	uint32_t patch_id;
	uint16_t loc_x;
	uint16_t loc_y;
};

class scenario_serialize : public serialize_base
{
public:
	enum class serialize_state : uint8_t
	{
		INIT,
		VERSION,
		COMMAND,
		END,
		ERROR,
	};
	enum command_res : uint8_t
	{
		INVALID,
		VALID,
		FINAL,
		CMD_INST,
		CMD_PATCH,
		CMD_UNKNOWN,
	};
	scenario_serialize();
	~scenario_serialize() override;

	static constexpr std::string_view
	get_cost_str(cost_type a) noexcept;
	static constexpr cost_type
	get_cost_type(std::string_view a) noexcept;

	/// @brief Resets class, including memory.  Must use between seperate
	/// read/writes, needed for memory managment.
	void
	reset();

	/// @return the current state of scenario read/write
	serialize_state
	state() const noexcept
	{
		return m_state;
	}

	void
	set_scenario_filename(std::filesystem::path&& filename)
	{
		set_filename(std::move(filename));
	}
	const std::filesystem::path&
	get_scenario_filename() const noexcept
	{
		return get_filename();
	}

	void
	set_map_filename(std::filesystem::path&& filename)
	{
		m_map_filename = std::move(filename);
	}
	const std::filesystem::path&
	get_map_filename() const noexcept
	{
		return m_map_filename;
	}

	void
	set_relative_map_filename(const std::filesystem::path& filename);

	void
	set_version(scenario_version version) noexcept
	{
		m_version = version;
	}
	scenario_version
	get_version() const noexcept
	{
		return m_version;
	}

	/// @brief the types of dist used, only relivent for version2
	std::span<const std::string_view>
	get_cost_strings() const noexcept
	{
		return m_cost_strings;
	}
	std::span<const cost_type>
	get_cost_type() const noexcept
	{
		return m_cost_type;
	}
	std::span<const double>
	get_cost_value() const noexcept
	{
		return m_cost_value;
	}
	int
	find_cost_index(cost_type c) const noexcept
	{
		auto it = std::find(m_cost_type.begin(), m_cost_type.end(), c);
		return it != m_cost_type.end()
		    ? static_cast<int>(it - m_cost_type.begin())
		    : -1;
	}
	int
	find_cost_index(std::string_view c) const noexcept
	{
		auto it = std::find(m_cost_strings.begin(), m_cost_strings.end(), c);
		return it != m_cost_strings.end()
		    ? static_cast<int>(it - m_cost_strings.begin())
		    : -1;
	}

	uint32_t
	get_map_width() const noexcept
	{
		return m_map_width;
	}
	int32_t
	get_map_height() const noexcept
	{
		return m_map_height;
	}

	void
	set_force_int(bool v) noexcept
	{
		m_force_int = v;
	}
	bool
	get_force_int() noexcept
	{
		return m_force_int;
	}

	void
	close() override;

	virtual int
	last_command_type() const;

	/// @brief reads in file version information, and sets version (accessible
	/// via get_version())
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{}, else failure
	/// @pre state() == serialize_state::INIT (returns errc otherwise)
	virtual std::expected<void,std::errc>
	read_version(std::istream* in = nullptr);
	/// @brief read header (without version) information.
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{}, else failure
	/// @pre state() == serialize_state::VERSION (returns errc otherwise)
	///
	/// With VERSION1: peeks first instance to gain map name
	/// With VERSION2: gets map width/height, available costs and patch
	/// filename
	virtual std::expected<void,std::errc>
	read_header(std::istream* in = nullptr);

	/// @brief read header as VERSION_1, does not consider the state or
	/// version. use read_header for checks instead.
	std::expected<void,std::errc>
	read_header_v1(std::istream* in = nullptr);
	/// @brief read header as VERSION_2, does not consider the state or
	/// version. use read_header for checks instead.
	std::expected<void,std::errc>
	read_header_v2(std::istream* in = nullptr);

	/// @brief gets the next command type
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return a pair with a value from command_res and std::errc for success
	/// (else error)
	/// @pre state() == serialize_state::COMMAND (returns error otherwise)
	///
	/// With VERSION_1:
	///   Returns CMD_INST or FINAL if no more commands are present
	/// With VERSION_2:
	///   Returns command based on last_command_type(), or FINAL if not
	///   present. Default expects CMD_INST or CMD_PATCH.
	std::expected<int, std::errc>
	next_command_type(std::istream* in = nullptr);
	/// @brief skips the next count number of commands
	/// @param count number of commands to skip
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{}, else failure
	/// @pre state() == serialize_state::COMMAND (returns errc otherwise)
	std::expected<int,std::errc>
	skip_commands(int count = 1, std::istream* in = nullptr);

	/// @brief reads an instance line and stores results in scenario_instance
	/// @param inst where to store the instance data read in
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return a pair with a value from command_res and std::errc for success
	/// (else error)
	/// @pre state() == serialize_state::COMMAND (returns error otherwise)
	///
	/// if next_command_type().first == CMD_INST, then reads the instance,
	/// otherwise returns CMD_? dependent on the type of command, or FINAL.
	/// With get_version() == VERSION_1:
	///   Returns VALID for success, FINAL for no more commands, and INVALID
	///   for invalid instance.
	/// With get_version() == VERSION_2:
	///   Returns VALID for success, FINAL for no more commands, and INVALID
	///   for invalid instance, or last_command_type() (only CMD_PATCH for
	///   standard v2 scenario) of type of command.
	///
	/// INVALID return without error code means success in reading command, but
	/// command has invalid parameters. Main checks are non-finite floats
	/// (start/goal/cost), mismatch width/height, or out of bounds start/goal.
	/// If get_force_int() == true, also checks start/goal are integers within
	/// the grid, otherwise allows float also within the grid (allows x ==
	/// width() or y == height()).
	virtual std::expected<int, std::errc>
	read_instance_line(scenario_instance& inst, std::istream* in = nullptr);

	/// @brief as VERSION_1 with read_instance_line, does not check
	/// pre-conditions
	std::expected<int, std::errc>
	read_instance_line_v1(scenario_instance& inst, std::istream* in = nullptr);
	/// @brief as VERSION_2 with read_instance_line, does not check
	/// pre-conditions
	std::expected<int, std::errc>
	read_instance_line_v2(scenario_instance& inst, std::istream* in = nullptr);

	/// @brief reads a patch line and stores results in scenario_patch
	/// @param patch where to store the patch data read in (not grid)
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return a pair with a value from command_res and std::errc for success
	/// (else error)
	/// @pre state() == serialize_state::COMMAND (returns error otherwise)
	///
	/// if next_command_type().first == CMD_PATCH, then reads the instance,
	/// otherwise returns CMD_? dependent on the type of command, or FINAL.
	/// With get_version() == VERSION_2:
	///   Returns VALID for success, FINAL for no more commands, INVALID if
	///   location is out of grid bounds, or last_command_type() (only CMD_INST
	///   for standard v2 scenario) of type of command.
	virtual std::expected<int, std::errc>
	read_patch_line(scenario_patch& patch, std::istream* in = nullptr);

	/// @brief as VERSION_2 with read_patch_line, does not check pre-conditions
	std::expected<int, std::errc>
	read_patch_line_v2(scenario_patch& patch, std::istream* in = nullptr);

protected:
	/// @return a owned version of str
	std::string_view
	copy_string(std::string_view str);

protected:
	serialize_state m_state    = serialize_state::INIT;
	scenario_version m_version = scenario_version::UNKNOWN;
	bool m_force_int           = false;
	std::filesystem::path m_map_filename;
	uint32_t m_map_width  = 0;
	uint32_t m_map_height = 0;
	int32_t m_inst_at     = 0;

	// dynamic data
	std::pmr::monotonic_buffer_resource m_dyn_res;
	std::pmr::monotonic_buffer_resource m_string_res;
	// distance types
	std::pmr::vector<std::string_view> m_cost_strings; ///< names read in
	std::pmr::vector<cost_type>
	    m_cost_type; ///< cost_type of index (corrisponding dist_strings and
	                 ///< dist_value)
	std::pmr::vector<double> m_cost_value; ///< distance value stored, will be
	                                       ///< placed in dynamic_scenario

	std::string m_command_type; ///< last cost type
};

constexpr std::string_view
scenario_serialize::get_cost_str(cost_type a) noexcept
{
	switch(a)
	{
	case cost_type::G_8C_NCC:
		return "8c-ncc";
	case cost_type::G_8C_CC:
		return "8c-cc";
	case cost_type::G_4C:
		return "4c";
	case cost_type::AA_NCC:
		return "aa-ncc";
	case cost_type::AA_CC:
		return "aa-cc";
	default:
		return std::string_view();
	}
}
constexpr cost_type
scenario_serialize::get_cost_type(std::string_view a) noexcept
{
	if(a == "8c-ncc") return cost_type::G_8C_NCC;
	if(a == "8c-cc") return cost_type::G_8C_CC;
	if(a == "4c") return cost_type::G_4C;
	if(a == "aa-ncc") return cost_type::AA_NCC;
	if(a == "aa-cc") return cost_type::AA_CC;
	return cost_type::OTHER;
}

} // namespace warthog::io

#endif // WARTHOG_IO_SCENARIO_H
