#include <warthog/domain/gridmap.h>

#include <bit>
#include <cassert>
#include <cstring>
#include <fstream>
#include <numeric>
#include <warthog/io/grid.h>

namespace warthog::domain
{

gridmap::gridmap() = default;

gridmap::gridmap(uint32_t h, uint32_t w) : header_(h, w, "octile")
{
	this->init_db();
}

gridmap::gridmap(std::istream& input)
{
	setup_stream_(input);
}

gridmap::gridmap(io::bittable_serialize& parser)
{
	setup_ser_(parser);
}

gridmap::gridmap(std::filesystem::path&& filename)
{
	filename_ = std::move(filename);
	std::ifstream in(filename_);
	setup_stream_(in);
}

gridmap::gridmap(const std::filesystem::path& filename)
    : gridmap(std::filesystem::path(filename))
{ }

gridmap::gridmap(const char* filename)
    : gridmap(std::filesystem::path(filename))
{ }

void
gridmap::setup(uint32_t h, uint32_t w)
{
	header_.height_ = h;
	header_.width_  = w;
	this->init_db();
}

void
gridmap::load(std::istream& input)
{
	setup_stream_(input);
}
void
gridmap::load(io::bittable_serialize& parser)
{
	setup_ser_(parser);
}
void
gridmap::load(std::filesystem::path&& filename)
{
	filename_ = std::move(filename);
	std::ifstream in(filename_);
	setup_stream_(in);
}

void
gridmap::load(const std::filesystem::path& filename)
{
	load(std::filesystem::path(filename));
}

void
gridmap::load(const char* filename)
{
	load(std::filesystem::path(filename));
}

void
gridmap::setup_stream_(std::istream& in)
{
	io::bittable_serialize parser;
	parser.open_read(&in);
	if(parser.read_header() != std::errc{})
		throw std::runtime_error("invalid grid format");
	setup_ser_(parser);
}

void
gridmap::setup_ser_(io::bittable_serialize& parser)
{
	if(parser.read_grid_header() != std::errc{})
		throw std::runtime_error("invalid grid format");
	this->header_.type_   = "octile";
	this->header_.width_  = parser.get_dim().width;
	this->header_.height_ = parser.get_dim().height;

	init_db();
	if(parser.read_grid_data(*this, 0, PADDED_ROWS) != std::errc{})
		throw std::runtime_error("invalid grid format");
	// calculate traversable
	num_traversable_ = static_cast<uint32_t>(std::transform_reduce(
	    db_.get(), db_.get() + db_size_, static_cast<int>(0),
	    std::plus<uint32_t>(), &std::popcount<dbword>));
}

void
gridmap::init_db()
{
	// when storing the grid we pad the edges of the map with
	// zeroes. this eliminates the need for bounds checking when
	// fetching the neighbours of a node.
	uint32_t store_width, store_height;
	store_height = this->header_.height_ + 2 * PADDED_ROWS;

	// calculate # of extra/redundant padding bits required,
	// per row, to align map width with dbword size
	store_width = this->header_.width_ + 1;
	if((store_width % 64) != 0)
	{
		store_width = (this->header_.width_ / 64 + 1) * 64;
	}
	this->padding_per_row_ = store_width - this->header_.width_;

	this->dbheight_  = store_height;
	this->dbwidth_   = store_width >> warthog::LOG2_DBWORD_BITS;
	this->dbwidth64_ = store_width >> 6;
	// the +8 is to allow unaligned access past the end
	this->db_size_ = bittable::calc_array_size(store_width, store_height) + 8;

	// create a one dimensional dbword array to store the grid
	this->db_ = std::make_unique<warthog::dbword[]>(db_size_);
	bittable::setup(this->db_.get(), store_width, store_height);
	fill(0);

	this->max_id_ = this->dbheight_ * this->dbwidth_ - 1;
}

gridmap::~gridmap() = default;

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
