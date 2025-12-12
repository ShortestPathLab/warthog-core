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
#include <bitset>
#include <array>
#include <vector>
#include <memory_resource>

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

class scenario_serialize
{
public:
	static constexpr size_t max_line_length = 2000;
	scenario_serialize();
	~scenario_serialize();

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
	/// @brief calls read_version then read_header for full header
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @return success std::errc{} (0)
	std::errc read_version_header(std::istream* in = nullptr);

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

protected:
	std::pair<std::istream*, std::errc> get_instream(std::istream* in) noexcept
	{
		if (in == nullptr)
			in = m_scenario_in;
		if (in == nullptr || !in->good())
			return {nullptr, std::errc::io_error};
		return {in, {}};
	}
	std::pair<std::ostream*, std::errc> get_outstream(std::ostream* out) noexcept
	{
		if (out == nullptr)
			out = m_scenario_out;
		if (out == nullptr || !out->good())
			return {nullptr, std::errc::io_error};
		return {out, {}};
	}
	std::pair<std::string_view, std::errc> readline(std::istream* in);

protected:
	scenario_version m_version = scenario_version::version1;
	std::filesystem::path m_scenario_filename;
	std::filesystem::path m_map_filename;
	std::istream* m_scenario_in = nullptr;
	std::ostream* m_scenario_out = nullptr;
	std::bitset<(size_t)dist_type::dist_count> m_dist;

	// dynamic data
	std::pmr::monotonic_buffer_resource m_dyn_res;
	std::pmr::string m_line;
	std::pmr::vector<std::string_view> m_dist_strings;
	std::pmr::vector<int16_t> m_dist_id;
};

} // namespace warthog::util

#endif // WARTHOG_IO_SCENARIO_H
