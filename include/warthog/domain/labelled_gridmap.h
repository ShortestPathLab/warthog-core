#ifndef WARTHOG_DOMAIN_LABELLED_GRIDMAP_H
#define WARTHOG_DOMAIN_LABELLED_GRIDMAP_H

// domains/labelled_gridmap.h
//
// A gridmap with weights/labels. This data structure supports
// labels on grid edges as well as vertices.
// This implementation stores the map as a flat 1d array of "cell"
// objects. We add some padding around the map to simplify access
// operations:
//  - a terminator character is added to indicate end-of-row.
//  - a line of terminator characters are added before the first row.
//  - a line of terminator characters are added after the last row.
//
// @author: dharabor
// @created: 2018-11-08
//

#include <warthog/constants.h>
#include <warthog/util/gm_parser.h>
#include <warthog/util/helpers.h>

#include <cassert>
#include <climits>
#include <cstring>
#include <stdint.h>

namespace warthog::domain
{

template<class CELL>
class labelled_gridmap
{
public:
	labelled_gridmap(unsigned int h, unsigned int w) : header_(h, w, "octile")
	{
		this->init_db();
	}

	labelled_gridmap(const char* filename)
	{
		util::gm_parser parser(filename);
		header_ = parser.get_header();
		strcpy(filename_, filename);
		init_db();

		for(unsigned int i = 0; i < parser.get_num_tiles(); i++)
		{
			CELL cell = parser.get_tile_at(i);
			;
			set_label(uint32_t{to_padded_id(pack_id{i})}, cell);
			assert(get_label(uint32_t{to_padded_id(pack_id{i})}) == cell);
		}
	}

	~labelled_gridmap() { delete[] db_; }

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

	CELL&
	get_label(uint32_t padded_id)
	{
		return db_[padded_id];
	}

	// set the label associated with the padded coordinate pair (x, y)
	void
	set_label(uint32_t x, uint32_t y, CELL label)
	{
		this->set_label(y * padded_width_ + x, label);
	}

	void
	set_label(uint32_t padded_id, CELL label)
	{
		db_[padded_id] = label;
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
	mem() const noexcept
	{
		return sizeof(*this) + sizeof(CELL) * db_size_;
	}

private:
	char filename_[256];
	util::gm_header header_;
	CELL* db_;

	uint32_t db_size_;
	uint32_t padding_per_row_;
	uint32_t padded_rows_before_first_row_;
	uint32_t padded_rows_after_last_row_;
	uint32_t padded_width_;
	uint32_t padded_height_;

	labelled_gridmap(const labelled_gridmap<CELL>& other) { }
	labelled_gridmap&
	operator=(const labelled_gridmap<CELL>& other)
	{
		return *this;
	}

	void
	init_db()
	{
		// when storing the grid we pad the edges of the map.
		// this eliminates the need for bounds checking when
		// fetching the neighbours of a node.
		this->padded_rows_before_first_row_ = 3;
		this->padded_rows_after_last_row_ = 3;
		this->padding_per_row_ = 1;

		this->padded_width_ = this->header_.width_ + this->padding_per_row_;
		this->padded_height_ = this->header_.height_
		    + this->padded_rows_after_last_row_
		    + this->padded_rows_before_first_row_;

		this->db_size_ = this->padded_height_ * padded_width_;

		// create a one dimensional dbword array to store the grid
		this->db_ = new CELL[db_size_];

		for(uint32_t i = 0; i < this->db_size_; i++)
		{
			this->db_[i] = 0;
		}
	}
};

// vertex-labelled gridmap
using vl_gridmap = labelled_gridmap<warthog::dbword>;

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_LABELLED_GRIDMAP_H
