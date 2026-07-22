#ifndef WARTHOG_IO_GRID_H
#define WARTHOG_IO_GRID_H

/// @file grid.h
///
/// Read utility for gridmap.
///
/// Supported MovingAI map format.  Read format spec:
/// https://movingai.com/benchmarks/formats.html
///
/// @author: Ryan Hechenberger
/// @created: 2025-06-01

#include "serialize_base.h"

#include <warthog/limits.h>
#include <warthog/memory/bittable.h>

#include <iomanip>
#include <stdexcept>
#include <vector>

namespace warthog::io
{

/// @brief Standard traversable terrain type from gridmap_cell
/// @param c
/// @return ".G" return true, false otherwise
constexpr bool
gridmap_cell_traversable(gridmap_cell c) noexcept
{
	switch(c)
	{
	case gridmap_cell::TERRAIN:
	case gridmap_cell::TERRAIN_2:
		return true;
	default:
		return false;
	}
}
/// @return char c is traversable, as gridmap_cell_traversable((gridmap_cell)c)
constexpr bool
gridmap_cell_traversable(char c) noexcept
{
	return gridmap_cell_traversable(static_cast<gridmap_cell>(c));
}

/// @brief max grid size
inline constexpr uint32_t GRID_MAX_SIZE = 15'000;

/// @brief limit on max number of patches
inline constexpr uint32_t PATCH_COUNT_LIMIT = 10'000'000;

/// @brief the bittable serialize class, flexable read/write of
/// bittable/gridmap or similiar datatypes, see serialize_base for
/// how files are read.
class bittable_serialize : public serialize_base
{
public:
	bittable_serialize();

	static constexpr uint32_t patch_auto
	    = std::numeric_limits<uint32_t>::max();

	/// @return the grid dimension, either as last read grid from file or set
	/// by user for writing
	memory::bittable_dimension
	get_dim() const noexcept
	{
		return m_dim;
	}
	/// @brief sets the grid dimension, throws if out of range
	bool
	set_dim(uint32_t width, uint32_t height)
	{
		if(bool bad_width = width <= 0 || width > GRID_DIMENSION_MAX,
		   bad_height     = height <= 0 || height > GRID_DIMENSION_MAX;
		   bad_width || bad_height)
		{
			return false;
		}
		m_dim.width  = width;
		m_dim.height = height;
		return true;
	}

	/// @return the type/version of the file, default OCTILE
	bittable_type
	get_type() const noexcept
	{
		return m_type;
	}
	/// @brief sets the type/version to write to the file header, supported is
	/// octile/patch.
	/// @return true for success, false failure
	bool
	set_type(bittable_type type)
	{
		if(type != bittable_type::OCTILE && type != bittable_type::PATCH)
		{
			return false;
		}
		m_type = type;
		return true;
	}

	/// @brief get the number of patches in file
	uint32_t
	get_patch_amount() const noexcept
	{
		return m_patch_amount;
	}
	/// @brief set the number of patches (for writing)
	/// @return true for success, false failure
	bool
	set_patch_amount(uint32_t count)
	{
		if(count > PATCH_COUNT_LIMIT) { return false; }
		m_patch_amount = count;
		return true;
	}

	/// @brief number of patches for writing is dynamic (requires seekable
	/// file)
	/// @return true for success, false failure
	bool
	set_patch_auto()
	{
		m_patch_amount = patch_auto;
		return true;
	}

	/// @brief gets the number of patches that have been read/write
	uint32_t
	get_patch_count() const noexcept
	{
		return m_patch_count;
	}

	/// @brief override the patch id to write
	/// @return true for success, false failure
	bool
	set_patch_id(uint32_t id) noexcept
	{
		m_patch_id = id;
		return true;
	}
	/// @brief gets the id of last patch (usually get_patch_count())
	uint32_t
	get_patch_id() const noexcept
	{
		return m_patch_id;
	}

	/// @brief reads the map/patch file header, getting the type
	/// @param in alternative file stream to read from
	/// @return error code on failure
	///
	/// Reads the header line, `type octile` for bittable_type::OCTILE or
	/// `type patch` for bittable_type::PATCH, retrievable by get_type().
	/// For PATCH type, also reads following line for number of patches in
	/// file.
	std::expected<void, std::errc>
	read_header(std::istream* in = nullptr);

	/// @brief Reads the grids' header, getting width/height up to the map
	/// data.
	/// @param in alternative filestream to read from
	/// @return error code on failure
	/// @pre get_type() matches the format of file.
	std::expected<void, std::errc>
	read_grid_header(std::istream* in = nullptr);

	/// @brief Reads the grids data and stores it into a bittable, expects size
	/// from get_dim()
	/// @param table bittable derived type to store, must be init
	/// @param offset_x offset of top-left in table to copy grid to
	/// @param offset_y offset of top-left in table to copy grid to
	/// @param in alternative filestream to read from
	/// @return error code on failure
	/// @pre table must be init and large enough to store whole grid (including
	/// from offset)
	template<typename BitTable>
	std::expected<void, std::errc>
	read_grid_data(
	    BitTable& table, uint32_t offset_x = 0, uint32_t offset_y = 0,
	    std::istream* in = nullptr);

	/// @brief Reads the raw rows (char) from map into a 1D array, expects size
	/// from get_dim()
	/// @param buffer the buffer
	/// @param in alternative filestream to read from
	/// @return error code on failure
	/// @pre buffer must be large enough to store width x height characters
	/// from get_dim()
	///
	/// Reads row by row from the top left, writing into buffer.
	/// Data is tightly packed, with no delimited between rows of size width.
	/// Characters are as defined by the MovingAI spec, use
	/// gridmap_cell_traversable(c) to determine traversability if applicable.
	std::expected<void, std::errc>
	read_grid_raw(std::span<char> buffer, std::istream* in = nullptr);

	/// @brief Writes out file header
	/// @param out alternative filestream to write to
	/// @return error code on failure
	/// @pre get_type()==bittable_type::OCTILE ||
	/// get_type()==bittable_type::PATCH
	///
	/// Writes out the header for either map (OCTILE) or patch (PATCH) file.
	/// For PATCH, set_patch_amount is needed in advanced, alternatively
	/// set_patch_auto() does not require knowing in advanced, but file stream
	/// must be seekable for this option and write_end() must be called at the
	/// end.
	std::expected<void, std::errc>
	write_header(std::ostream* out = nullptr);

	/// @brief Writes out a bittable (cropped) as a grid
	/// @param table table to write
	/// @param offset_x top-left x to crop to
	/// @param offset_y top-left y to crop to
	/// @param blocker char to represent a blocker (=0)
	/// @param traversable char to represent a non-blocker (!=0)
	/// @param out alternative filestream to write to
	/// @return error code on failure
	/// @pre get_dim() != {}
	///
	/// Writes out a bittable to file, cropped to region (offset_x,offset_y) --
	/// (offset_x+get_dim().width,offset_y+get_dim().height).
	/// User must use set_dim(width,height) before calling this function to set
	/// the output grid width/height.
	template<typename BitTable>
	std::expected<void, std::errc>
	write_grid_data(
	    BitTable& table, uint32_t offset_x = 0, uint32_t offset_y = 0,
	    gridmap_cell blocker     = gridmap_cell::OUT_OF_BOUNDS,
	    gridmap_cell traversable = gridmap_cell::TERRAIN,
	    std::ostream* out        = nullptr);

	/// @brief Writes out a grid from user-given buffer
	/// @param buffer grid to write (top-left start at buffer[0])
	/// @param out alternative filestream to write to
	/// @return error code on failure
	/// @pre get_dim() != {}
	///
	/// Similar to write_grid_data, except user provides the map data.
	/// User must use set_dim(width,height) before calling this function to set
	/// the output grid width/height.
	std::expected<void, std::errc>
	write_grid_raw(std::span<char> buffer, std::ostream* out = nullptr);

	/// @brief Perform end of file work, only required for set_patch_auto()
	/// @param out alternative filestream to write to
	/// @return error code on failure
	std::expected<void, std::errc>
	write_end(std::ostream* out = nullptr);

protected:
	memory::bittable_dimension m_dim = {};
	bittable_type m_type             = bittable_type::NONE;
	uint32_t m_patch_amount          = 0;
	uint32_t m_patch_count           = 0;
	uint32_t m_patch_id              = 0;
	std::streampos m_patch_auto_pos  = 0;
};

template<typename BitTable>
std::expected<void, std::errc>
bittable_serialize::read_grid_data(
    BitTable& table, uint32_t offset_x, uint32_t offset_y, std::istream* in)
{
	// check table
	const memory::bittable_dimension dim      = table.dim();
	const memory::bittable_dimension read_dim = m_dim;
	// detect for overflow
	if(offset_x >= dim.width || read_dim.width + offset_x > dim.width)
		return std::unexpected(std::errc::argument_out_of_domain);
	if(offset_y >= dim.height || read_dim.height + offset_y > dim.height)
		return std::unexpected(std::errc::argument_out_of_domain);

	uint32_t bit_id
	    = static_cast<uint32_t>(table.xy_to_id(offset_x, offset_y));
	const uint32_t bit_row_offset = dim.width - read_dim.width;

	if(auto r = get_istream(in); r)
		in = *r;
	else
		return std::unexpected(r.error());

	std::string_view line;
	std::string_view token;
	for(uint32_t y = 0; y < read_dim.height; ++y, bit_id += bit_row_offset)
	{
		// read row
		if(auto r = readline(in); r)
			line = *r;
		else
			return std::unexpected(r.error());
		parser par(line);
		if(!par.next(token).eof()) { return std::unexpected(par.error()); }
		if(token.size() != read_dim.width)
			return std::unexpected(std::errc::argument_out_of_domain);
		// copy row to table
		for(uint32_t x = 0; x < read_dim.width; ++x, ++bit_id)
		{
			table.set(
			    static_cast<BitTable::id_type>(bit_id),
			    gridmap_cell_traversable(token[x]) ? 1 : 0);
		}
	}

	return {};
}

template<typename BitTable>
std::expected<void, std::errc>
bittable_serialize::write_grid_data(
    BitTable& table, uint32_t offset_x, uint32_t offset_y,
    gridmap_cell blocker, gridmap_cell traversable, std::ostream* out)
{
	// check table
	const memory::bittable_dimension dim       = table.dim();
	const memory::bittable_dimension write_dim = m_dim;
	// detect for overflow
	if(offset_x >= dim.width || write_dim.width == 0
	   || write_dim.width + offset_x > dim.width)
		return std::unexpected(std::errc::argument_out_of_domain);
	if(offset_y >= dim.height || write_dim.height == 0
	   || write_dim.height + offset_y > dim.height)
		return std::unexpected(std::errc::argument_out_of_domain);

	uint32_t bit_id
	    = static_cast<uint32_t>(table.xy_to_id(offset_x, offset_y));
	const uint32_t bit_row_offset = dim.width - write_dim.width;

	if(auto r = get_ostream(out); r)
		out = *r;
	else
		return std::unexpected(r.error());

	if(m_type == bittable_type::PATCH)
	{
		if(!(*out << "patch " << m_patch_id++ << '\n'))
			return std::unexpected(std::errc::io_error);
	}
	if(!(*out << "height " << write_dim.height << "\nwidth " << write_dim.width
	          << "\nmap\n"))
		return std::unexpected(std::errc::io_error);

	std::array<char, 2048> buffer;
	std::unique_ptr<char[]> buffer_dyn;
	std::span<char> line_buffer;
	if(write_dim.width < 2048)
	{
		// use stack buffer
		line_buffer = std::span<char>(buffer.data(), write_dim.width + 1);
	}
	else
	{
		// too large, use dynamic buffer
		buffer_dyn  = std::make_unique<char[]>(write_dim.width + 1);
		line_buffer = std::span<char>(buffer_dyn.get(), write_dim.width + 1);
	}
	// set end to newline
	line_buffer[write_dim.width] = '\n';

	for(uint32_t y = 0; y < write_dim.height; ++y, bit_id += bit_row_offset)
	{
		// copy row to buffer
		for(uint32_t x = 0; x < write_dim.width; ++x, ++bit_id)
		{
			line_buffer[x] = table.get(static_cast<BitTable::id_type>(bit_id))
			    ? (char)traversable
			    : (char)blocker;
		}
		// write row
		if(!(*out << std::string_view(line_buffer.data(), line_buffer.size())))
			return std::unexpected(std::errc::io_error);
	}
	return {};
}

} // namespace warthog::memory

#endif // WARTHOG_IO_GRID_H
