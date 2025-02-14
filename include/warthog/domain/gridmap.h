#ifndef WARTHOG_DOMAIN_GRIDMAP_H
#define WARTHOG_DOMAIN_GRIDMAP_H

// gridmap.h
//
// A uniform cost gridmap implementation.  The map is stored in
// a compact matrix form. Nodes are represented as single bit quantities.
//
// NB: To allow efficient lookups this implementation uses a padding scheme
// that adds extra elements to the end of each row and also introduces an
// empty row immediately before and immediately after the start and end of
// the gridmap data.
// Padding allows us to refer to tiles and their neighbours by their indexes
// in a one dimensional array and also to avoid range checks when trying to
// identify invalid neighbours of tiles on the edge of the map.
//
// @author: dharabor
// @created: 08/08/2012
//

#include "grid.h"
#include <warthog/constants.h>
#include <warthog/memory/bittable.h>
#include <warthog/util/gm_parser.h>
#include <warthog/util/helpers.h>
#include <warthog/util/cast.h>

#include <cassert>
#include <climits>
#include <cstdint>
#include <limits>
#include <cstring>
#include <bit>

namespace warthog::domain
{

struct gridmap_slider
{
	const warthog::dbword* loc;
	uint32_t width8;
	uint32_t width8_bits; /// only used for end-user

	constexpr void adj_bytes(int i) noexcept { loc += i; }

	// returns rows as:
	// [0] = middle
	// [1] = above (-y)
	// [2] = below (+y)
	std::array<uint64_t, 3>
	get_neighbours_64bit_le() const noexcept
	{
		std::array<uint64_t, 3> return_value;
		std::memcpy(&return_value[0], loc, sizeof(uint64_t));
		std::memcpy(&return_value[1], loc - width8, sizeof(uint64_t));
		std::memcpy(&return_value[2], loc + width8, sizeof(uint64_t));

		if constexpr (std::endian::native == std::endian::big)
		{
			// big endian, perform byte swap
			return_value[0] = util::byteswap_u64(return_value[0]);
			return_value[1] = util::byteswap_u64(return_value[1]);
			return_value[2] = util::byteswap_u64(return_value[2]);
		}

		return return_value;
	}

#ifdef WARTHOG_INT128_ENABLED
	std::array<unsigned __int128, 3>
	get_neighbours_128bit_le() const noexcept
	{
		using int128 = unsigned __int128;
		std::array<int128, 3> return_value;
		std::memcpy(&return_value[0], loc, sizeof(int128));
		std::memcpy(&return_value[1], loc - width8, sizeof(int128));
		std::memcpy(&return_value[2], loc + width8, sizeof(int128));

		if constexpr (std::endian::native == std::endian::big)
		{
			// big endian, perform byte swap
			return_value[0] = util::byteswap_u128(return_value[0]);
			return_value[1] = util::byteswap_u128(return_value[1]);
			return_value[2] = util::byteswap_u128(return_value[2]);
		}

		return return_value;
	}
#endif
};

constexpr uint32_t GRID_ID_MAX = std::numeric_limits<uint32_t>::max();
class gridmap : public memory::bittable<pad_id, warthog::dbword>
{
public:
	gridmap(uint32_t height, uint32_t width);
	gridmap(const char* filename);
	gridmap(const gridmap&) = delete;
	~gridmap();

	/// The number of padded rows before and after
	static constexpr uint32_t PADDED_ROWS = 3;

	gridmap&
	operator=(const gridmap&)
	    = delete;

	// here we convert from the coordinate space of
	// the original grid to the coordinate space of db_.
	pad_id
	to_padded_id(pack_id node_id) const noexcept
	{
		assert(header_.width_ != 0);
		return pad_id{
		    uint32_t{node_id} +
		    // padded rows before the actual map data starts
		    PADDED_ROWS * width() +
		    // padding from each row of data before this one
		    (uint32_t{node_id} / header_.width_) * padding_per_row_};
	}

	// here we convert from the coordinate space of
	// the original grid to the coordinate space of db_.
	pad_id
	to_padded_id_from_unpadded(uint32_t x, uint32_t y) const noexcept
	{
		return pad_id{(y + PADDED_ROWS) * width() + x};
	}
	pad_id
	to_padded_id_from_padded(uint32_t x, uint32_t y) const noexcept
	{
		return pad_id{y * width() + x};
	}

	void
	to_unpadded_xy(pack_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		y = uint32_t{grid_id} / header_.width_;
		x = uint32_t{grid_id} % header_.width_;
		assert(x < header_.width_ && y < header_.height_);
	}

	void
	to_unpadded_xy(pad_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		to_padded_xy(grid_id, x, y);
		y -= PADDED_ROWS;
		assert(x < header_.width_ && y < header_.height_);
	}

	void
	to_padded_xy(pad_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		y = uint32_t{grid_id} / width();
		x = uint32_t{grid_id} % width();
		assert(x < width() && y < height());
	}

	pack_id
	to_unpadded_id(pad_id grid_id) const noexcept
	{
		assert(width() != 0);
		return pack_id{
		    uint32_t{grid_id} -
		    // padding from each row of data
		    (uint32_t{grid_id} / width()) * padding_per_row_ -
		    // padded rows before the actual map data starts, use header_width
		    // as the padded width is already removed
		    PADDED_ROWS * header_.width_};
	}
	pack_id
	to_unpadded_id_from_unpadded(uint32_t x, uint32_t y) const noexcept
	{
		return pack_id{y * header_.width_ + x};
	}

	// get the immediately adjacent neighbours of @param node_id
	// neighbours from the row above node_id are stored in
	// @param tiles[0], neighbours from the same row in tiles[1]
	// and neighbours from the row below in tiles[2].
	// In each case the bits for each neighbour are in the three
	// lowest positions of the byte.
	// position :0 is the nei in direction NW, :1 is N and :2 is NE
	void
	get_neighbours(pad_id grid_id, uint8_t tiles[3]) const
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset
		    = static_cast<uint32_t>(grid_id.id & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex
		    = static_cast<uint32_t>(grid_id.id >> warthog::LOG2_DBWORD_BITS);

		// compute dbword indexes for tiles immediately above
		// and immediately below node_id
		uint32_t pos1 = dbindex - dbwidth_;
		uint32_t pos2 = dbindex;
		uint32_t pos3 = dbindex + dbwidth_;

		// read from the byte just before node_id and shift down until the
		// nei adjacent to node_id is in the lowest position
		tiles[0]
		    = (uint8_t)(*((uint32_t*)(db_ + (pos1 - 1))) >> (bit_offset + 7));
		tiles[1]
		    = (uint8_t)(*((uint32_t*)(db_ + (pos2 - 1))) >> (bit_offset + 7));
		tiles[2]
		    = (uint8_t)(*((uint32_t*)(db_ + (pos3 - 1))) >> (bit_offset + 7));
	}

	// fetches a contiguous set of tiles from three adjacent rows. each row is
	// 32 tiles long. the middle row begins with tile grid_id_p. the other
	// tiles are from the row immediately above and immediately below
	// grid_id_p.
	void
	get_neighbours_32bit(pad_id grid_id, uint32_t tiles[3]) const
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset
		    = static_cast<uint32_t>(grid_id.id & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex
		    = static_cast<uint32_t>(grid_id.id >> warthog::LOG2_DBWORD_BITS);

		// compute dbword indexes for tiles immediately above
		// and immediately below node_id
		uint32_t pos1 = dbindex - dbwidth_;
		uint32_t pos2 = dbindex;
		uint32_t pos3 = dbindex + dbwidth_;

		// read 32bits of memory; grid_id_p is in the
		// lowest bit position of tiles[1]
		tiles[0] = (uint32_t)(*((uint64_t*)(db_ + pos1)) >> (bit_offset));
		tiles[1] = (uint32_t)(*((uint64_t*)(db_ + pos2)) >> (bit_offset));
		tiles[2] = (uint32_t)(*((uint64_t*)(db_ + pos3)) >> (bit_offset));
	}

	// similar to get_neighbours_32bit but grid_id_p is placed into the
	// upper bit of the return value. this variant is useful when jumping
	// toward smaller memory addresses (i.e. west instead of east).
	void
	get_neighbours_upper_32bit(pad_id grid_id, uint32_t tiles[3]) const
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset
		    = static_cast<uint32_t>(grid_id.id & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex
		    = static_cast<uint32_t>(grid_id.id >> warthog::LOG2_DBWORD_BITS);

		// start reading from a prior index. this way everything
		// up to grid_id_p is cached.
		dbindex -= 4;

		// compute dbword indexes for tiles immediately above
		// and immediately below node_id
		uint32_t pos1 = dbindex - dbwidth_;
		uint32_t pos2 = dbindex;
		uint32_t pos3 = dbindex + dbwidth_;

		// read 32bits of memory; grid_id_p is in the
		// highest bit position of tiles[1]
		tiles[0] = (uint32_t)(*((uint64_t*)(db_ + pos1)) >> (bit_offset + 1));
		tiles[1] = (uint32_t)(*((uint64_t*)(db_ + pos2)) >> (bit_offset + 1));
		tiles[2] = (uint64_t)(*((uint64_t*)(db_ + pos3)) >> (bit_offset + 1));
	}

	// fetches a contiguous set of tiles from three adjacent rows.
	// the middle row contains tile grid_id_p.
	// the other tiles are from the row above and below grid_id_p.
	void
	get_neighbours_64bit(pad_id grid_id, uint64_t tiles[3]) const
	{
		// convert grid_id_p into a 64bit db_ index.
		uint32_t dbindex = static_cast<uint32_t>(grid_id.id >> 6);

		// compute 64bit dbword indexes for the tiles
		// above and below node_id
		uint32_t pos1 = dbindex - dbwidth64_;
		uint32_t pos2 = dbindex;
		uint32_t pos3 = dbindex + dbwidth64_;

		// read 64bits of tile data from each of the three rows
		tiles[0] = *((uint64_t*)(db_) + pos1);
		tiles[1] = *((uint64_t*)(db_) + pos2);
		tiles[2] = *((uint64_t*)(db_) + pos3);
	}

	// fetches a contiguous set of tiles from three adjacent rows.
	// the middle row contains tile grid_id_p.
	// the other tiles are from the row above and below grid_id_p.
	// returns rows in little endian. Will perform byte_swap in big endian systems.
	gridmap_slider
	get_neighbours_slider(pad_id grid_id) const
	{
		return {data() + static_cast<uint32_t>(grid_id.id >> base_bit_width),
			static_cast<uint32_t>(dbwidth_),
			static_cast<uint32_t>(grid_id.id & base_bit_mask)};
	}

	// get the label associated with the padded coordinate pair (x, y)
	bool
	get_label(uint32_t x, uint32_t y) const
	{
		return this->get_label(to_padded_id_from_padded(x, y));
	}

	// change: now does not check bounds
	warthog::dbword
	get_label(pad_id grid_id) const
	{
		return bittable::get(grid_id);
	}

	// get a pointer to the word that contains the label of node @grid_id_p
	const warthog::dbword*
	get_mem_ptr(pad_id grid_id) const
	{
		return data() + id_split(grid_id).first;
	}

	// set the label associated with the padded coordinate pair (x, y)
	void
	set_label(uint32_t x, uint32_t y, bool label)
	{
		this->set_label(to_padded_id_from_unpadded(x, y), label);
	}

	void
	set_label(pad_id grid_id, bool label)
	{
		bittable::set(grid_id, label);
	}

	uint32_t
	padded_mapsize() const noexcept
	{
		return width() * height();
	}

	uint32_t
	header_height() const noexcept
	{
		return this->header_.height_;
	}

	uint32_t
	header_width() const noexcept
	{
		return this->header_.width_;
	}

	const char*
	filename() const noexcept
	{
		return this->filename_;
	}

	uint32_t
	get_num_traversable_tiles() const noexcept
	{
		return num_traversable_;
	}

	void
	print(std::ostream&);

	void
	printdb(std::ostream& out);

	size_t
	mem() const noexcept
	{
		return sizeof(*this) + sizeof(warthog::dbword) * db_size_;
	}

private:
	using bittable::setup;

	warthog::util::gm_header header_;
	warthog::dbword* db_;
	char filename_[256];

	uint32_t dbwidth_;
	uint32_t dbwidth64_;
	uint32_t dbheight_;
	uint32_t db_size_;
	uint32_t padding_per_row_;
	uint32_t padding_column_above_;
	uint32_t max_id_;
	uint32_t num_traversable_;

	void
	init_db();
};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_GRIDMAP_H
