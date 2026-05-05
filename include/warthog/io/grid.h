#ifndef WARTHOG_IO_GRID_H
#define WARTHOG_IO_GRID_H

// io/grid.h
//
// Read utility for gridmap.
//
// Supported MovingAI map format.  Read format spec: https://movingai.com/benchmarks/formats.html
//
// @author: Ryan Hechenberger
// @created: 2025-06-01
//

#include "serialize_base.h"

#include <warthog/limits.h>
#include <warthog/memory/bittable.h>

#include <iomanip>
#include <stdexcept>
#include <vector>

namespace warthog::io
{

enum class bittable_type : uint8_t
{
	OCTILE,
	PATCH,
	OTHER,
	NONE,
};

enum class gridmap_cell : char
{
	TERRAIN = '.',
	TERRAIN_2 = 'G',
	OUT_OF_BOUNDS = '@',
	OUT_OF_BOUNDS_2 = 'O',
	TREES = 'T',
	SWAMP = 'S',
	WATER = 'W',
};

/// @brief Standard traversable terrain type from gridmap_cell
/// @param c 
/// @return ".G" return true, false otherwise
constexpr inline bool gridmap_cell_traversable(gridmap_cell c) noexcept
{
	switch (c) {
	case gridmap_cell::TERRAIN:
	case gridmap_cell::TERRAIN_2:
		return true;
	default:
		return false;
	}
}
/// @return char c is traversable, as gridmap_cell_traversable((gridmap_cell)c)
constexpr inline bool gridmap_cell_traversable(char c) noexcept
{
	return gridmap_cell_traversable(static_cast<gridmap_cell>(c));
}

/// @brief Max grid size
inline constexpr uint32_t GRID_MAX_SIZE = 15'000;

/// @brief Limit on max number of patches
inline constexpr uint32_t PATCH_COUNT_LIMIT = 10'000'000;

/// @brief The bittable serialize class, flexable read/write of bittable/gridmap or
///        similiar datatypes
class bittable_serialize : public serialize_base
{
public:
	/// @return the grid dimension, either as last read grid from file or set by user for writing
	memory::bittable_dimension
	get_dim() const noexcept
	{
		return m_dim;
	}
	/// @brief sets the grid dimension, throws if out of range
	void
	set_dim(uint32_t width, uint32_t height)
	{
		if(bool bad_width = width <= 0 || width > GRID_DIMENSION_MAX,
		   bad_height     = height <= 0 || height > GRID_DIMENSION_MAX;
		   bad_width || bad_height)
		{
			throw std::out_of_range(bad_width ? "width" : "height");
		}
		m_dim.width  = width;
		m_dim.height = height;
	}

	/// @return the type/version of the file, default OCTILE
	bittable_type
	get_type() const noexcept
	{
		return m_type;
	}
	/// @brief Sets the type/version to write to the file header, supported is octile/patch.
	///        Throws on unsupported type.
	void
	set_type(bittable_type type)
	{
		if(type != bittable_type::OCTILE && type != bittable_type::PATCH)
		{
			throw std::out_of_range("type");
		}
		m_type = type;
	}

	/// @brief get the number of patches in file
	uint32_t get_patch_amount() const noexcept
	{
		return m_patch_amount;
	}
	/// @brief set the number of patches (for writing)
	void set_patch_amount(uint32_t count)
	{
		if(count > PATCH_COUNT_LIMIT)
		{
			throw std::out_of_range("count");
		}
		m_patch_amount = count;
	}
	/// @brief gets the number of patches that have been read/write
	uint32_t get_patch_count() const noexcept
	{
		return m_patch_count;
	}
	/// @brief gets the id of last patch (usually get_patch_count())
	uint32_t get_patch_id() const noexcept
	{
		return m_patch_id;
	}
	/// @brief gets the number of patches that have been read
	void set_patch_id(uint32_t id) noexcept
	{
		m_patch_id = id;
	}

	/// @brief Reads the map/patch file header, getting the type
	/// @param in alternative filestream to read from
	/// @return value init on success, error code on failure
	///
	/// Reads the header line, `type octile` for bittable_type::OCTILE or
	/// `type patch` for bittable_type::PATCH, retrivable by get_type().
	/// For PATCH type, also reads following line for number of patches in file.
	std::errc
	read_header(std::istream* in = nullptr);

	/// @brief Reads the grids header, getting width/height up to the map data.
	/// @param in alternative filestream to read from
	/// @return value init on success, error code on failure
	/// @pre get_type() matches the format of file.
	std::errc
	read_grid_header(std::istream* in = nullptr);

	/// @brief Reads the grids data and stores it into a bittable, expects size from get_dim()
	/// @param table bittable derived type to store, must be init
	/// @param offset_x offset of top-left in table to copy grid to
	/// @param offset_y offset of top-left in table to copy grid to
	/// @param in alternative filestream to read from
	/// @return value init on success, error code on failure
	/// @pre table must be init and large enough to store whole grid (including from offset)
	template<typename BitTable>
	std::errc
	read_grid_data(
	    BitTable& table, uint32_t offset_x = 0,
	    uint32_t offset_y = 0, std::istream* in = nullptr);

	/// @brief Reads the raw rows (char) from map into a 1D array, expects size from get_dim()
	/// @param buffer the buffer
	/// @param in alternative filestream to read from
	/// @return value init on success, error code on failure
	/// @pre buffer must be large enough to store width x height characters from get_dim()
	///
	/// Reads row by row from the top left, writing into buffer.
	/// Data is tightly packed, with no delimited between rows of size width.
	/// Characters are as defined by the MovingAI spec, use gridmap_cell_traversable(c)
	/// to determine traversability if applicable.
	std::errc
	read_grid_raw(
	    std::span<char> buffer, std::istream* in = nullptr);

protected:
	memory::bittable_dimension m_dim = {};
	bittable_type m_type             = bittable_type::NONE;
	uint32_t m_patch_amount          = 0;
	uint32_t m_patch_count           = 0;
	uint32_t m_patch_id             = 0;
};

template<typename BitTable>
std::errc
bittable_serialize::read_grid_data(
    BitTable& table, uint32_t offset_x, uint32_t offset_y, std::istream* in)
{
	// check table
	const memory::bittable_dimension dim      = table.dim();
	const memory::bittable_dimension read_dim = m_dim;
	// detect for overflow
	if(offset_x >= dim.width || read_dim.width + offset_x > dim.width)
		return std::errc::argument_out_of_domain;
	if(offset_y >= dim.height || read_dim.height + offset_y > dim.height)
		return std::errc::argument_out_of_domain;
	uint32_t bit_id
	    = static_cast<uint32_t>(table.xy_to_id(offset_x, offset_y));
	const uint32_t bit_row_offset = dim.width - read_dim.width;

	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		return err;
	}
	std::string_view line;
	std::string_view token;

	for(uint32_t y = 0; y < read_dim.height; ++y, bit_id += bit_row_offset)
	{
		// read row
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
		parser par(line);
		if (!par.next(token).eof())
		{
			return par.error();
		}
		if (token.size() != read_dim.width)
			return std::errc::argument_out_of_domain;
		// copy row to table
		for(uint32_t x = 0; x < read_dim.width; ++x, ++bit_id)
		{
			table.set(static_cast<BitTable::id_type>(bit_id), gridmap_cell_traversable(token[x]) ? 1 : 0);
		}
	}

	return std::errc{};
}

} // namespace warthog::memory

#endif // WARTHOG_IO_GRID_H
