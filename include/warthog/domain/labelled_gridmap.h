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
// @author: dharabor & Ryan Hechenberger
// @created: 2018-11-08
//

#include "grid.h"
#include <warthog/constants.h>
#include <warthog/io/bittable_serialize.h>
#include <warthog/util/gm_header.h>
#include <warthog/util/helpers.h>

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <span>

namespace warthog::domain
{

template<class CELL>
class labelled_gridmap
{
public:
	labelled_gridmap() = default;
	labelled_gridmap(uint32_t h, uint32_t w) : header_(h, w, "octile")
	{
		this->init_db();
	}
	labelled_gridmap(std::istream& input) { setup_stream_(input); }
	labelled_gridmap(io::bittable_serialize& parser) { setup_ser_(parser); }
	labelled_gridmap(std::filesystem::path&& filename)
	{
		filename_ = std::move(filename);
		std::ifstream in(filename_);
		setup_stream_(in);
	}
	labelled_gridmap(const std::filesystem::path& filename)
	    : labelled_gridmap(std::filesystem::path(filename))
	{ }
	labelled_gridmap(const char* filename)
	    : labelled_gridmap(std::filesystem::path(filename))
	{ }
	labelled_gridmap(const labelled_gridmap&) = delete;
	~labelled_gridmap()                       = default;

	/// The number of padded rows before and after
	static constexpr uint32_t PADDED_ROWS = 3;

	labelled_gridmap&
	operator=(const labelled_gridmap&)
	    = delete;

	void
	setup(uint32_t h, uint32_t w)
	{
		header_.height_ = h;
		header_.width_  = w;
		this->init_db();
	}
	void
	load(std::istream& input)
	{
		setup_stream_(input);
	}
	void
	load(io::bittable_serialize& parser)
	{
		setup_ser_(parser);
	}
	void
	load(std::filesystem::path filename)
	{
		filename_ = std::move(filename);
		std::ifstream in(filename_);
		setup_stream_(in);
	}
	void
	load(const char* filename)
	{
		load(std::filesystem::path(filename));
	}

	/// @brief convert unpadded id to padded id
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

	/// @brief convert unpadded (x,y) to a padded id
	pad_id
	to_padded_id_from_unpadded(uint32_t x, uint32_t y) const noexcept
	{
		return pad_id{(y + PADDED_ROWS) * width() + x};
	}
	/// @brief convert padded (x,y) to a padded id
	pad_id
	to_padded_id_from_padded(uint32_t x, uint32_t y) const noexcept
	{
		return pad_id{y * width() + x};
	}

	/// @brief convert unpadded id to unpadded (x,y)
	void
	to_unpadded_xy(pack_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		y = uint32_t{grid_id} / header_.width_;
		x = uint32_t{grid_id} % header_.width_;
		assert(x < header_.width_ && y < header_.height_);
	}

	/// @brief convert padded id to unpadded (x,y)
	void
	to_unpadded_xy(pad_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		to_padded_xy(grid_id, x, y);
		y -= PADDED_ROWS;
		assert(x < header_.width_ && y < header_.height_);
	}

	/// @brief convert padded (x,y) to unpadded (x,y)
	void
	to_unpadded_xy_from_padded(
	    uint32_t padded_x, uint32_t padded_y, uint32_t& x,
	    uint32_t& y) const noexcept
	{
		y = padded_y - PADDED_ROWS;
		x = padded_x;
		assert(x < header_.width_ && y < header_.height_);
	}

	/// @brief convert padded id to padded (x,y)
	void
	to_padded_xy(pad_id grid_id, uint32_t& x, uint32_t& y) const noexcept
	{
		y = uint32_t{grid_id} / width();
		x = uint32_t{grid_id} % width();
		assert(x < width() && y < height());
	}

	/// @brief convert unpadded (x,y) to padded (x,y)
	void
	to_padded_xy_from_unpadded(
	    uint32_t unpadded_x, uint32_t unpadded_y, uint32_t& x,
	    uint32_t& y) const noexcept
	{
		y = unpadded_y + PADDED_ROWS;
		x = unpadded_x;
		assert(x < width() && y < height());
	}

	/// @brief convert padded id to unpadded id
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
	/// @brief convert unpadded (x,y) to unpadded id
	pack_id
	to_unpadded_id_from_unpadded(uint32_t x, uint32_t y) const noexcept
	{
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

	const std::filesystem::path
	filename() const noexcept
	{
		return this->filename_;
	}

	uint32_t
	mem() const noexcept
	{
		return sizeof(*this) + sizeof(CELL) * db_size_;
	}

protected:
	void
	setup_stream_(std::istream& in);
	void
	setup_ser_(io::bittable_serialize& parser);

	util::gm_header header_ = {};
	std::unique_ptr<CELL[]> db_;
	std::filesystem::path filename_;

	uint32_t db_size_                      = 0;
	uint32_t padding_per_row_              = 0;
	uint32_t padded_rows_before_first_row_ = 0;
	uint32_t padded_rows_after_last_row_   = 0;
	uint32_t padded_width_                 = 0;
	uint32_t padded_height_                = 0;

	void
	init_db();
};

template<typename CELL>
inline void
labelled_gridmap<CELL>::setup_stream_(std::istream& in)
{
	io::bittable_serialize parser;
	if(!parser.open_read(&in)) throw std::runtime_error("invalid grid stream");
	if(!parser.read_header()) throw std::runtime_error("invalid grid format");
	setup_ser_(parser);
}

template<typename CELL>
inline void
labelled_gridmap<CELL>::setup_ser_(io::bittable_serialize& parser)
{
	if(!parser.read_grid_header())
		throw std::runtime_error("invalid grid format");
	this->header_.type_   = "octile";
	this->header_.width_  = parser.get_dim().width;
	this->header_.height_ = parser.get_dim().height;

	init_db();
	// read raw data to buffer
	std::unique_ptr<char[]> buffer_v
	    = std::make_unique<char[]>(this->db_size_);
	std::span<char> buffer(buffer_v.get(), this->db_size_);
	if(!parser.read_grid_raw(buffer))
		throw std::runtime_error("invalid grid format");
	// copy buffet to db
	std::copy_n(buffer.data(), buffer.size(), this->db_.get());
}

template<typename CELL>
inline void
labelled_gridmap<CELL>::init_db()
{
	// when storing the grid we pad the edges of the map with
	// zeroes. this eliminates the need for bounds checking when
	// fetching the neighbours of a node.
	uint32_t store_width, store_height;
	store_height = this->header_.height_ + 2 * PADDED_ROWS;

	// calculate # of extra/redundant padding bits required,
	// per row, to align map width with dbword size
	store_width = this->header_.width_ + 1;
	if((store_width % 8) != 0)
	{
		store_width = (this->header_.width_ / 8 + 1) * 8;
	}
	this->padded_width_                 = store_width;
	this->padded_height_                = store_height;
	this->padded_rows_before_first_row_ = PADDED_ROWS;
	this->padded_rows_after_last_row_   = PADDED_ROWS;
	this->padding_per_row_              = store_width - this->header_.width_;

	this->db_size_ = store_width * store_height;

	// create a one dimensional dbword array to store the grid
	this->db_ = std::make_unique<warthog::dbword[]>(db_size_);
	std::memset(this->db_.get(), 0, this->db_size_);
}

// vertex-labelled gridmap
using vl_gridmap = labelled_gridmap<warthog::dbword>;

} // namespace warthog::domain

#endif // WARTHOG_DOMAIN_LABELLED_GRIDMAP_H
