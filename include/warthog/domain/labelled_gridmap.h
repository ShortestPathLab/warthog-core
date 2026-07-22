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
#include <warthog/io/log.h>
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
	labelled_gridmap(uint32_t h, uint32_t w) : header_{h, w}
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

	bool
	load(std::istream& input)
	{
		return setup_stream_(input);
	}
	bool
	load(io::bittable_serialize& parser)
	{
		return setup_ser_(parser);
	}
	bool
	load(std::filesystem::path filename)
	{
		filename_ = std::move(filename);
		std::ifstream in(filename_);
		return setup_stream_(in);
	}
	bool
	load(const char* filename)
	{
		return load(std::filesystem::path(filename));
	}

	bool
	save(std::ostream& input, bool padding = false)
	{
		io::bittable_serialize parser;
		if(!parser.open_write(&input))
		{
			WARTHOG_GERROR("gridmap save failed with input stream");
			return false;
		}
		return save(parser, padding);
	}
	bool
	save(io::bittable_serialize& parser, bool padding = false);
	bool
	save(const std::filesystem::path& filename, bool padding = false)
	{
		io::bittable_serialize parser;
		parser.set_filename(std::filesystem::path(filename));
		if(!parser.open_write())
		{
			WARTHOG_GERROR_FMT(
			    "gridmap save failed to write to file \"{}\"",
			    filename.string());
			return false;
		}
		return save(parser, padding);
	}
	bool
	save(const char* filename, bool padding = false)
	{
		io::bittable_serialize parser;
		parser.set_filename(filename);
		if(!parser.open_write())
		{
			WARTHOG_GERROR_FMT(
			    "gridmap save failed to write to file \"{}\"", filename);
			return false;
		}
		return save(parser, padding);
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

	operator bool() const noexcept { return static_cast<bool>(db_); }

protected:
	bool
	setup_stream_(std::istream& in);
	bool
	setup_ser_(io::bittable_serialize& parser);

	struct
	{
		uint32_t height_;
		uint32_t width_;
	} header_ = {};
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
inline bool
labelled_gridmap<CELL>::save(io::bittable_serialize& parser, bool padding)
{
	if(!*this)
	{
		WARTHOG_GERROR("gridmap save failed due to empty gridmap");
		return false;
	}
	const uint32_t lwidth  = padding ? width() : header_width();
	const uint32_t lheight = padding ? height() : header_height();
	if(!parser.set_type(warthog::io::bittable_type::OCTILE))
	{
		WARTHOG_GERROR("gridmap save failed to set type");
		return false;
	}
	if(!parser.set_dim(lwidth, lheight))
	{
		WARTHOG_GERROR_FMT(
		    "gridmap save invalid dimensions {}x{}", lwidth, lheight);
		return false;
	}

	// write gridmap data
	if(auto r = parser.write_header(); !r)
	{
		WARTHOG_GERROR_FMT(
		    "gridmap save failed header write errc={}", (int)r.error());
		return false;
	}

	// setup and write raw grid data
	std::unique_ptr<char[]> buffer_v
	    = std::make_unique<char[]>(lwidth * lheight);
	std::span<char> buffer(buffer_v.get(), lwidth * lheight);
	pad_id row_id    = pad_id::zero();
	pad_id buffer_id = pad_id::zero();
	pad_id row_id_end(lwidth * lheight);
	if(padding)
	{
		// convert 0 to '@'
		std::ranges::fill(buffer, '@');
		// output had buffer, only write in unbuffered area
		buffer_id = to_padded_id(static_cast<pack_id>(buffer_id));
	}
	else
	{
		// update pad_id to only cover unpadded area of grid
		row_id     = to_padded_id(static_cast<pack_id>(row_id));
		row_id_end = to_padded_id(static_cast<pack_id>(row_id_end));
	}
	// copy unpadded grid to (un)padded output grid
	const uint32_t pwidth = width();
	const uint32_t hwidth = header_width();
	std::span<CELL> grid(this->db_.get(), this->db_size_);
	for(; row_id.id < row_id_end.id;
	    row_id.id += pwidth, buffer_id.id += lwidth)
	{
		std::ranges::copy(
		    grid.subspan(row_id.id, hwidth),
		    buffer.subspan(buffer_id.id, hwidth));
	}
	if(auto r = parser.write_grid_raw(buffer); !r)
	{
		WARTHOG_GERROR_FMT(
		    "gridmap save failed grid write errc={}", (int)r.error());
		return false;
	}
	if(auto r = parser.write_end(); !r)
	{
		WARTHOG_GERROR_FMT(
		    "gridmap save failed finalize errc={}", (int)r.error());
		return false;
	}

	return true;
}

template<typename CELL>
inline bool
labelled_gridmap<CELL>::setup_stream_(std::istream& in)
{
	io::bittable_serialize parser;
	if(!parser.open_read(&in)) return false;
	if(!parser.read_header()) return false;
	return setup_ser_(parser);
}

template<typename CELL>
inline bool
labelled_gridmap<CELL>::setup_ser_(io::bittable_serialize& parser)
{
	if(!parser.read_grid_header()) return false;
	const uint32_t lwidth = parser.get_dim().width;
	const uint64_t lsize  = (uint64_t)lwidth * parser.get_dim().height;
	this->header_.width_  = lwidth;
	this->header_.height_ = parser.get_dim().height;

	init_db();
	// read raw data to buffer
	std::unique_ptr<char[]> buffer_v = std::make_unique<char[]>(lsize);
	std::span<char> buffer(buffer_v.get(), lsize);
	if(!parser.read_grid_raw(buffer)) return false;
	// copy buffet to db, add padding
	for(pack_id row_id(0); row_id.id < lsize; row_id.id += lwidth)
	{
		std::ranges::copy(
		    buffer.subspan(row_id.id, lwidth),
		    this->db_.get() + to_padded_id(row_id).id);
	}
	return true;
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
