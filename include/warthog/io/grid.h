#ifndef WARTHOG_IO_GRID_H
#define WARTHOG_IO_GRID_H

#include <warthog/memory/bittable.h>
#include <warthog/limits.h>
#include <stdexcept>
#include <iomanip>

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

enum class bittable_cell : uint8_t
{
	BLOCKER,
	TRAVERSABLE,
	UNKNOWN
};

class bittable_serialize
{
public:
	memory::bittable_dimension get_dim() const noexcept { return m_dim; }
	bittable_type get_type() const noexcept { return m_type; }

	static constexpr bool is_traversable(char c) noexcept
	{
		switch (c) {
		case '.':
		case 'G':
			return true;
		default:
			return false;
		}
	}
	static constexpr bittable_cell cell_type(char c) noexcept
	{
		switch (c) {
		case '.':
		case 'G':
			return bittable_cell::TRAVERSABLE;
		case '@':
		case 'O':
		case 'S':
		case 'T':
		case 'W':
			return bittable_cell::BLOCKER;
		default:
			return bittable_cell::UNKNOWN;
		}
	}

	void set_dim(uint32_t width, uint32_t height)
	{
		if (bool bad_width = width <= 0 || width > GRID_DIMENSION_MAX, bad_height = height <= 0 || height > GRID_DIMENSION_MAX;
			bad_width || bad_height) {
			throw std::out_of_range(bad_width ? "width" : "height");
		}
		m_dim.width = width;
		m_dim.height = height;
	}
	void set_type(bittable_type type)
	{
		if (static_cast<uint32_t>(type) > static_cast<uint32_t>(bittable_type::NONE)) {
			throw std::out_of_range("type");
		}
		m_type = type;
	}

	bool read_header(std::istream& in);

	template <typename BitTable>
	bool read_map(std::istream& in, BitTable& table, uint32_t offset_x = 0, uint32_t offset_y = 0);

protected:
	memory::bittable_dimension m_dim = {};
	bittable_type m_type = bittable_type::AUTO;
	uint32_t m_patch_count = 0;
};

template <typename BitTable>
bool bittable_serialize::read_map(std::istream& in, BitTable& table, uint32_t offset_x, uint32_t offset_y)
{
	const memory::bittable_dimension dim = table.dim();
	const memory::bittable_dimension read_dim = m_dim;
	table.fill(0); // set whole table to 0 (blocker)
	// detect for overflow
	if (offset_x >= dim.width || read_dim.width + offset_x > dim.width)
		return false;
	if (offset_y >= dim.height || read_dim.height + offset_y > dim.height)
		return false;
	assert(read_dim.width <= GRID_DIMENSION_MAX);
	if (read_dim.width > GRID_DIMENSION_MAX)
		return false; // shound never happen
	uint32_t bit_id = static_cast<uint32_t>(table.xy_to_id(offset_x, offset_y));
	const uint32_t bit_row_offset = dim.width - read_dim.width;
	char buffer[(GRID_DIMENSION_MAX + 16) & ~7ull];
	static_assert(sizeof(buffer) / sizeof(char) >= GRID_DIMENSION_MAX);
	for (uint32_t y = 0; y < read_dim.height; ++y, bit_id += bit_row_offset) {
		in >> std::ws;
		in.read(buffer, read_dim.width);
		for (uint32_t x = 0; x < read_dim.width; ++x, ++bit_id) {
			auto cell = cell_type(buffer[x]);
			if (cell == bittable_cell::UNKNOWN)
				return false;
			else if (cell == bittable_cell::TRAVERSABLE)
				table.bit_or(static_cast<BitTable::id_type>(bit_id), 1);
		}
	}
	return true;
}

} // namespace warthog::memory

#endif // WARTHOG_IO_GRID_H
