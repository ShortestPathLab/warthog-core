#ifndef WARTHOG_IO_GRID_H
#define WARTHOG_IO_GRID_H

// io/grid.h
//
// Read utility for gridmap.
//
//Supported MovingAI map format.  Read format spec: https://movingai.com/benchmarks/formats.html
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
	AUTO,
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
constexpr inline bool gridmap_cell_traversable(char c) noexcept
{
	return gridmap_cell_traversable(static_cast<gridmap_cell>(c));
}

inline constexpr uint32_t GRID_MAX_SIZE = 15'000;

class bittable_serialize : public serialize_base
{
public:
	memory::bittable_dimension
	get_dim() const noexcept
	{
		return m_dim;
	}
	bittable_type
	get_type() const noexcept
	{
		return m_type;
	}

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
	void
	set_type(bittable_type type)
	{
		if(static_cast<uint32_t>(type)
		   > static_cast<uint32_t>(bittable_type::NONE))
		{
			throw std::out_of_range("type");
		}
		m_type = type;
	}

	std::errc
	read_header(std::istream* in = nullptr);

	std::errc
	read_grid_header(std::istream* in = nullptr);

	template<typename BitTable>
	std::errc
	read_grid_data(
	    BitTable& table, uint32_t offset_x = 0,
	    uint32_t offset_y = 0, std::istream* in = nullptr);

	std::errc
	read_grid_raw(
	    std::vector<char>& raw_data, std::istream* in = nullptr);

protected:
	memory::bittable_dimension m_dim = {};
	bittable_type m_type             = bittable_type::AUTO;
	uint32_t m_patch_count           = 0;
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
	for(uint32_t y = 0; y < read_dim.height; ++y, bit_id += bit_row_offset)
	{
		// read row
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
		auto& iss = line_stream(line);
		if (!(iss >> m_token) || !line_stream_eof())
		{
			return std::errc::io_error;
		}
		if (m_token.size() != read_dim.width)
			return std::errc::argument_out_of_domain;
		// copy row to table
		for(uint32_t x = 0; x < read_dim.width; ++x, ++bit_id)
		{
			if (gridmap_cell_traversable(m_token[x]))
				table.bit_or(static_cast<BitTable::id_type>(bit_id), 1);
		}
	}

	return std::errc{};
}

} // namespace warthog::memory

#endif // WARTHOG_IO_GRID_H
