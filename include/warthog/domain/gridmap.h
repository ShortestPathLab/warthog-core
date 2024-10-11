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
#include <warthog/util/gm_parser.h>
#include <warthog/util/helpers.h>

#include <climits>
#include <cstdint>
#include <limits>

namespace warthog::domain
{

constexpr uint32_t GRID_ID_MAX = std::numeric_limits<uint32_t>::max();
class gridmap
{
public:
	gridmap(uint32_t height, uint32_t width);
	gridmap(const char* filename);
	~gridmap();

	// here we convert from the coordinate space of
	// the original grid to the coordinate space of db_.
	pad_id
	to_padded_id(pack_id node_id)
	{
		return pad_id{
		    uint32_t{node_id} +
		    // padded rows before the actual map data starts
		    padded_rows_before_first_row_ * padded_width_ +
		    // padding from each row of data before this one
		    (node_id.id / header_.width_) * padding_per_row_};
	}

	// here we convert from the coordinate space of
	// the original grid to the coordinate space of db_.
	pad_id
	to_padded_id(uint32_t x, uint32_t y)
	{
		return to_padded_id(pack_id{y * this->header_width() + x});
	}

	void
	to_unpadded_xy(pack_id grid_id_p, uint32_t& x, uint32_t& y)
	{
		y = uint32_t{grid_id_p} / padded_width_;
		x = uint32_t{grid_id_p} % padded_width_;
	}

	void
	to_unpadded_xy(pad_id grid_id_p, uint32_t& x, uint32_t& y)
	{
		uint32_t id = uint32_t{grid_id_p}
		    - padded_rows_before_first_row_ * padded_width_;
		y = id / padded_width_;
		x = id % padded_width_;
	}

	pack_id
	to_unpadded_id(pad_id padded_id)
	{
		uint32_t x, y;
		to_unpadded_xy(padded_id, x, y);
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
	get_neighbours(uint32_t grid_id_p, uint8_t tiles[3])
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset = (grid_id_p & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;

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
	get_neighbours_32bit(uint32_t grid_id_p, uint32_t tiles[3])
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset = (grid_id_p & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;

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
	get_neighbours_upper_32bit(uint32_t grid_id_p, uint32_t tiles[3])
	{
		// 1. calculate the dbword offset for the node at index grid_id_p
		// 2. convert grid_id_p into a dbword index.
		uint32_t bit_offset = (grid_id_p & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;

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
	get_neighbours_64bit(uint32_t grid_id_p, uint64_t tiles[3])
	{
		// convert grid_id_p into a 64bit db_ index.
		uint32_t dbindex = grid_id_p >> 6;

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

	// get the label associated with the padded coordinate pair (x, y)
	bool
	get_label(uint32_t x, uint32_t y)
	{
		return this->get_label(y * padded_width_ + x);
	}

	// TODO: for now, kept internal as uint32_t for smaller call size, decide
	// later
	warthog::dbword
	get_label(uint32_t grid_id_p)
	{
		// now we can fetch the label
		uint32_t bitmask = 1;
		bitmask <<= (grid_id_p & warthog::DBWORD_BITS_MASK);
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;
		if(dbindex > max_id_) { return 0; }
		return (db_[dbindex] & bitmask) != 0;
	}

	// get a pointer to the word that contains the label of node @grid_id_p
	warthog::dbword*
	get_mem_ptr(uint32_t grid_id_p)
	{
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;
		if(dbindex > max_id_) { return 0; }
		return &db_[dbindex];
	}

	// set the label associated with the padded coordinate pair (x, y)
	void
	set_label(uint32_t x, unsigned int y, bool label)
	{
		this->set_label(y * padded_width_ + x, label);
	}

	void
	set_label(uint32_t grid_id_p, bool label)
	{
		uint32_t dbindex = grid_id_p >> warthog::LOG2_DBWORD_BITS;
		uint32_t bitmask = 1u << (grid_id_p & warthog::DBWORD_BITS_MASK);

		if(dbindex > max_id_) { return; }

		if(label) { db_[dbindex] |= (warthog::dbword)bitmask; }
		else { db_[dbindex] &= (warthog::dbword)~bitmask; }
	}

	uint32_t
	padded_mapsize() const noexcept
	{
		return padded_width_ * padded_height_;
	}

	uint32_t
	height() const noexcept
	{
		return this->padded_height_;
	}

	uint32_t
	width() const noexcept
	{
		return this->padded_width_;
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

	// invert map (traversable tiles become obstacles and vice versa)
	void
	invert()
	{
		for(unsigned int i = 0; i < db_size_; i++)
		{
			db_[i] = (warthog::dbword)~db_[i];
		}
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
	warthog::util::gm_header header_;
	warthog::dbword* db_;
	char filename_[256];

	uint32_t dbwidth_;
	uint32_t dbwidth64_;
	uint32_t dbheight_;
	uint32_t db_size_;
	uint32_t padded_width_;
	uint32_t padded_height_;
	uint32_t padding_per_row_;
	uint32_t padding_column_above_;
	uint32_t padded_rows_before_first_row_;
	uint32_t padded_rows_after_last_row_;
	uint32_t max_id_;
	uint32_t num_traversable_;

	gridmap(const gridmap& other) { }
	gridmap&
	operator=(const gridmap& other)
	{
		return *this;
	}
	void
	init_db();
};

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_GRIDMAP_H
