#ifndef WARTHOG_UTIL_GM_PARSER_H
#define WARTHOG_UTIL_GM_PARSER_H

// gm_parser.h
//
// A parser for gridmap files written in Nathan Sturtevant's HOG format.
//
// @author: dharabor
// @created: 08/08/2012
//

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

namespace warthog::util
{

class gm_header
{
public:
	gm_header(unsigned int height, unsigned int width, const char* type)
	    : height_(height), width_(width), type_(type)
	{ }

	gm_header() : height_(0), width_(0), type_("") { }

	gm_header(const gm_header& other) { (*this) = other; }

	virtual ~gm_header() { }

	gm_header&
	operator=(const gm_header& other)
	{
		this->height_ = other.height_;
		this->width_  = other.width_;
		this->type_   = other.type_;
		return *this;
	}

	unsigned int height_;
	unsigned int width_;
	std::string type_;
};

class gm_parser
{
public:
	gm_parser(const char* filename);
	~gm_parser();

	inline gm_header
	get_header()
	{
		return this->header_;
	}

	inline size_t
	get_num_tiles()
	{
		return this->map_.size();
	}

	inline unsigned char
	get_tile_at(unsigned int index)
	{
		if(index >= this->map_.size())
		{
			std::cerr << "err; gm_parser::get_tile_at "
			             "invalid index "
			          << index << std::endl;
			exit(1);
		}
		return this->map_.at(index);
	}

private:
	gm_parser(const gm_parser&) { }
	gm_parser&
	operator=(const gm_parser&)
	{
		return *this;
	}

	void
	parse_header(std::fstream&);
	void
	parse_map(std::fstream&);

	std::vector<unsigned char> map_;
	gm_header header_;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_GM_PARSER_H
