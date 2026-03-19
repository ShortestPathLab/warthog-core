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

#include <cstdint>
#include <iomanip>
#include <filesystem>
#include <fstream>
#include <bitset>
#include <array>
#include <vector>
#include <memory_resource>
#include <cassert>
#include <sstream>

namespace warthog::io
{

enum class scenario_version : uint8_t
{
	version1,
	version2,
};
enum class dist_type : uint8_t
{
	octile_ncc,
	octile_cc,
	manhatten,
	anyangle_ncc,
	anyangle_cc,
	dist_count,
};

struct scenario_query
{
	int64_t bucket;
	std::string map;
	double start_x;
	double start_y;
	double goal_x;
	double goal_y;
	std::array<double, (size_t)dist_type::dist_count> dist;

	void reset()
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
	enum class serialize_state
	{
		init,
		version,
		header,
		query,
		error,
	};
	static constexpr size_t max_line_length = 2000;
	static constexpr size_t max_dimension = 15'000;
	scenario_serialize();
	~scenario_serialize();

	/// @brief Resets class, including memory.  Must use between seperate read/writes, needed for memory managment.
	void reset();

	void set_scenario_filename(std::filesystem::path&& filename)
	{
		m_scenario_filename = std::move(filename);
	}
	const std::filesystem::path& get_scenario_filename() const noexcept
	{
		return m_scenario_filename;
	}

	void set_map_filename(std::filesystem::path&& filename)
	{
		m_map_filename = std::move(filename);
	}
	const std::filesystem::path& get_map_filename() const noexcept
	{
		return m_map_filename;
	}

	void set_relative_map_filename(std::filesystem::path&& filename);

	void set_version(scenario_version version) noexcept
	{
		m_version = version;
		m_dist.reset();
		if (version == scenario_version::version1)
			m_dist.set(0, true);
	}
	scenario_version get_version() const noexcept
	{
		return m_version;
	}

	/// @brief the types of dist used, only relivent for version2, otherwise octile_ncc(0)
	/// @return bitset<dist_count>
	auto get_dist_types() const noexcept
	{
		return m_dist;
	}
	bool has_dist_type(dist_type d) const noexcept
	{
		assert((uint32_t)d < (uint32_t)dist_type::dist_count);
		return m_dist.test((uint32_t)d);
	}

	/// @brief opens scenario file get_scenario_filename() for reading
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	std::errc open_read(std::istream* scenario = nullptr);
	/// @brief opens scenario file get_scenario_filename() for writing
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	std::errc open_write(std::ostream* scenario = nullptr);

	/// @return if scenario is open for reading
	bool can_read(std::istream* in = nullptr)
	{
		if (in == nullptr)
			in = m_scenario_in;
		return in != nullptr && in->good();
	}
	/// @return if scenario is open for writing
	bool can_write(std::ostream* out = nullptr)
	{
		if (out == nullptr)
			out = m_scenario_out;
		return out != nullptr && out->good();
	}

	/// @brief reads in file version information, and sets version accessable via get_version()
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{} (0)
	std::errc read_version(std::istream* in = nullptr);
	/// @brief with header (without version) information.
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{} (0)
	///
	/// With version1: peeks first query to gain map name
	/// With version2: gets map width/height, available costs and patch filename
	std::errc read_header(std::istream* in = nullptr);

protected:
	std::pair<std::istream*, std::errc> get_istream(std::istream* in = nullptr) noexcept
	{
		if (in == nullptr)
			in = m_scenario_in;
		if (in == nullptr || !in->good())
			return {nullptr, std::errc::io_error};
		return {in, {}};
	}
	std::pair<std::ostream*, std::errc> get_ostream(std::ostream* out = nullptr) noexcept
	{
		if (out == nullptr)
			out = m_scenario_out;
		if (out == nullptr || !out->good())
			return {nullptr, std::errc::io_error};
		return {out, {}};
	}
	std::pair<std::string_view, std::errc> readline(std::istream* in);

	std::errc read_header_v1(std::istream* in = nullptr);
	std::errc read_header_v2(std::istream* in = nullptr);

	std::pair<bool,std::errc> read_query_line_v1(scenario_query& query);
	std::pair<bool,std::errc> read_query_line_v2(scenario_query& query);
	std::pair<bool,std::errc> read_patch_line_v2(scenario_patch& query);

protected:
	serialize_state m_state = serialize_state::init;
	scenario_version m_version = scenario_version::version1;
	std::filesystem::path m_scenario_filename;
	std::filesystem::path m_map_filename;
	std::istream* m_scenario_in = nullptr;
	std::ostream* m_scenario_out = nullptr;
	std::unique_ptr<std::ios_base> m_scenario_file;
	std::bitset<(size_t)dist_type::dist_count> m_dist;
	uint32_t m_map_width = 0;
	uint32_t m_map_height = 0;
	int32_t m_query_at = 0;

	// dynamic data
	std::pmr::monotonic_buffer_resource m_dyn_res;
	std::pmr::string m_line;
	std::pmr::vector<std::string_view> m_dist_strings;
	std::pmr::vector<int16_t> m_dist_id;

	// shared temp parameter
	// TODO: replace with a custom string stream that does not allocate memory
	std::istringstream m_iss;
	std::string m_token;
	scenario_query m_query;
};

} // namespace warthog::util

#endif // WARTHOG_IO_SCENARIO_H
