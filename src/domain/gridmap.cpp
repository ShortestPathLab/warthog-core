#include <warthog/domain/gridmap.h>

#include <cassert>
#include <cstring>

namespace warthog::domain
{

gridmap::gridmap(unsigned int h, unsigned int w) : header_(h, w, "octile")
{
	this->init_db();
}

gridmap::gridmap(const char* filename)
{
	strcpy(filename_, filename);
	warthog::util::gm_parser parser(filename);
	this->header_ = parser.get_header();

	init_db();
	// populate matrix
	num_traversable_ = 0;
	for(uint32_t i = 0; i < parser.get_num_tiles(); i++)
	{
		unsigned char c = parser.get_tile_at(i);
		switch(c)
		{
		case 'S':
		case 'W':
		case 'T':
		case '@':
		case 'O': // these terrain types are obstacles
			this->set_label(to_padded_id(pack_id{i}), 0);
			assert(this->get_label(to_padded_id(pack_id{i})) == 0);
			break;
		default: // everything else is traversable
			this->set_label(to_padded_id(pack_id{i}), 1);
			num_traversable_++;
			assert(this->get_label(to_padded_id(pack_id{i})) == 1);
			break;
		}
	}
}

void
gridmap::init_db()
{
	// when storing the grid we pad the edges of the map with
	// zeroes. this eliminates the need for bounds checking when
	// fetching the neighbours of a node.
	this->padded_rows_before_first_row_ = 3;
	this->padded_rows_after_last_row_ = 3;
	uint32_t store_width, store_height;
	store_height = this->header_.height_ + padded_rows_after_last_row_
	    + padded_rows_before_first_row_;

	// calculate # of extra/redundant padding bits required,
	// per row, to align map width with dbword size
	store_width = this->header_.width_ + 1;
	if((store_width % 64) != 0)
	{
		store_width = (this->header_.width_ / 64 + 1) * 64;
	}
	this->padding_per_row_ = store_width - this->header_.width_;

	this->dbheight_ = store_height;
	this->dbwidth_ = store_width >> warthog::LOG2_DBWORD_BITS;
	this->dbwidth64_ = store_width >> 6;
	this->db_size_ = bittable::calc_array_size(store_width, store_height);

	// create a one dimensional dbword array to store the grid
	this->db_ = new warthog::dbword[db_size_];
	bittable::setup(this->db_, store_width, store_height);
	fill(0);

	max_id_ = this->dbheight_ * this->dbwidth_ - 1;
}

gridmap::~gridmap()
{
	delete[] db_;
}

void
gridmap::print(std::ostream& out)
{
	out << "printing padded map" << std::endl;
	out << "-------------------" << std::endl;
	out << "type " << header_.type_ << std::endl;
	out << "height " << this->height() << std::endl;
	out << "width " << this->width() << std::endl;
	out << "map" << std::endl;
	for(unsigned int y = 0; y < this->height(); y++)
	{
		for(unsigned int x = 0; x < this->width(); x++)
		{
			warthog::dbword c = this->get_label(pad_id{y * this->width() + x});
			out << (c ? '.' : '@');
		}
		out << std::endl;
	}
}

} // namespace warthog::domain
