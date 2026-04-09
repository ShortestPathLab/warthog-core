#ifndef WARTHOG_IO_SCENARIO_H
#define WARTHOG_IO_SCENARIO_H

// io/scenario.h
//
// Read/write utilities for scenario files.
//
//	Supported formats for read:
//	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
//		  (fields: bucket,map,mapwidth,mapheight,sx,sy,gx,gy,distance)
//	    - DIMACS format (as at the 9th DIMACS Implementation Challenge)
//	      (fields: q [source-id] [target-id])
//
//	Supported formats for generate/write:
//	    - GPPC 1.0 format (as at 2012 Grid-based Path Planning Competition)
//      - Dynamic format (tbd link)
//
// @author: dharabor & Ryan Hechenberger
// @created: 2025-12-04
//

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <memory_resource>
#include <sstream>
#include <vector>
#include <span>
#include <compare>
#include <algorithm>

namespace warthog::io
{

enum class scenario_version : uint8_t
{
	UNKNOWN,
	VERSION_1,
	VERSION_2,
};
enum class cost_type : uint8_t
{
	G_8C_NCC,
	G_8C_CC,
	G_4C,
	AA_NCC,
	AA_CC,
	OTHER,
};

struct scenario_query
{
	int64_t bucket;
	std::string_view map;
	int32_t width;
	int32_t height;
	double start_x;
	double start_y;
	double goal_x;
	double goal_y;
	std::span<double> dist;

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

class scenario_serialize
{
public:
	enum class serialize_state : uint8_t
	{
		init,
		version,
		header,
		query,
		error,
	};
	enum query_res : uint8_t
	{
		valid,
		invalid,
		final,
		next_query,
		next_patch
	};
	static constexpr size_t max_line_length = 2000;
	scenario_serialize();
	~scenario_serialize();

	static constexpr std::string_view get_dist_str(cost_type a) noexcept;
	static constexpr cost_type get_cost_type(std::string_view a) noexcept;

	/// @brief Resets class, including memory.  Must use between seperate
	/// read/writes, needed for memory managment.
	void
	reset();

	void
	set_scenario_filename(std::filesystem::path&& filename)
	{
		m_scenario_filename = std::move(filename);
	}
	const std::filesystem::path&
	get_scenario_filename() const noexcept
	{
		return m_scenario_filename;
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

	/// @brief the types of dist used, only relivent for version2, otherwise
	/// octile_ncc(0)
	/// @return bitset<dist_count>
	std::span<const std::string_view>
	get_dist_strings() const noexcept
	{
		return m_dist_strings;
	}
	std::span<const cost_type>
	get_cost_type() const noexcept
	{
		return m_cost_type;
	}
	std::span<const double>
	get_dist_value() const noexcept
	{
		return m_dist_value;
	}
	int
	get_dist_index(cost_type d) const noexcept
	{
		auto it = std::find(m_cost_type.begin(), m_cost_type.end(), d);
		return it != m_cost_type.end() ? static_cast<int>(it - m_cost_type.begin()) : -1;
	}

	int32_t
	get_line_num() const noexcept
	{
		return m_line_num;
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
	std::string_view
	get_last_line() const noexcept
	{
		return m_line;
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

	/// @brief opens scenario file get_scenario_filename() for reading
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	std::errc
	open_read(std::istream* scenario = nullptr);
	/// @brief opens scenario file get_scenario_filename() for writing
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	std::errc
	open_write(std::ostream* scenario = nullptr);

	void
	close();

	/// @return if scenario is open for reading
	bool
	can_read(std::istream* in = nullptr)
	{
		if(in == nullptr) in = m_scenario_in;
		return in != nullptr && !in->bad();
	}
	/// @return if scenario is open for writing
	bool
	can_write(std::ostream* out = nullptr)
	{
		if(out == nullptr) out = m_scenario_out;
		return out != nullptr && !out->bad();
	}

	/// @brief reads in file version information, and sets version accessable
	/// via get_version()
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{} (0)
	std::errc
	read_version(std::istream* in = nullptr);
	/// @brief with header (without version) information.
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{} (0)
	///
	/// With VERSION1: peeks first query to gain map name
	/// With version2: gets map width/height, available costs and patch
	/// filename
	std::errc
	read_header(std::istream* in = nullptr);

	std::errc
	read_header_v1(std::istream* in = nullptr);
	std::errc
	read_header_v2(std::istream* in = nullptr);

	std::pair<query_res, std::errc>
	read_query_line(scenario_query& query, std::istream* in = nullptr);

	std::pair<query_res, std::errc>
	read_query_line_v1(scenario_query& query, std::istream* in = nullptr);
	std::pair<query_res, std::errc>
	read_query_line_v2(scenario_query& query, std::istream* in = nullptr);
	std::pair<query_res, std::errc>
	read_patch_line_v2(scenario_patch& query, std::istream* in = nullptr);

protected:
	std::pair<std::istream*, std::errc>
	get_istream(std::istream* in = nullptr) noexcept
	{
		if(in == nullptr) in = m_scenario_in;
		if(in == nullptr || !in->good()) return {nullptr, std::errc::io_error};
		return {in, {}};
	}
	std::pair<std::ostream*, std::errc>
	get_ostream(std::ostream* out = nullptr) noexcept
	{
		if(out == nullptr) out = m_scenario_out;
		if(out == nullptr || !out->good())
			return {nullptr, std::errc::io_error};
		return {out, {}};
	}
	std::istream&
	line_stream(std::string_view line);
	std::pair<std::string_view, std::errc>
	readline(std::istream* in);
	void
	unreadline(std::string_view line);

protected:
	serialize_state m_state    = serialize_state::init;
	scenario_version m_version = scenario_version::UNKNOWN;
	bool m_force_int           = false;
	std::filesystem::path m_scenario_filename;
	std::filesystem::path m_map_filename;
	std::unique_ptr<std::ios_base> m_scenario_stream;
	std::istream* m_scenario_in  = nullptr;
	std::ostream* m_scenario_out = nullptr;
	uint32_t m_map_width  = 0;
	uint32_t m_map_height = 0;
	int32_t m_query_at    = 0;
	int32_t m_line_num    = -1;

	// dynamic data
	std::pmr::monotonic_buffer_resource m_dyn_res;
	std::pmr::monotonic_buffer_resource m_string_res;
	char* m_line = nullptr; ///< sets from m_dyn_res
	std::pmr::string m_unget_line;
	// distance types
	std::pmr::vector<std::string_view> m_dist_strings; ///< names read in
	std::pmr::vector<cost_type> m_cost_type; ///< cost_type of index (corrisponding dist_strings and dist_value)
	std::pmr::vector<double> m_dist_value; ///< distance value stored, will be placed in dynamic_scenario

	// shared temp parameter
	// TODO: replace with a custom string stream that does not allocate memory
	std::istringstream m_iss;
	std::string m_token;
};

inline constexpr std::string_view scenario_serialize::get_dist_str(cost_type a) noexcept
{
	switch(a) {
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
inline constexpr cost_type scenario_serialize::get_cost_type(std::string_view a) noexcept
{
	if (a == "8c-ncc")
		return cost_type::G_8C_NCC;
	if (a == "8c-cc")
		return cost_type::G_8C_CC;
	if (a == "4c")
		return cost_type::G_4C;
	if (a == "aa-ncc")
		return cost_type::AA_NCC;
	if (a == "aa-cc")
		return cost_type::AA_CC;
	return cost_type::OTHER;
}

} // namespace warthog::util

#endif // WARTHOG_IO_SCENARIO_H
