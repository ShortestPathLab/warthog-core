#ifndef WARTHOG_UTIL_GM_PARSER_H
#define WARTHOG_UTIL_GM_PARSER_H

// gm_parser.h
//
// A header for gridmap.
//
// @author: dharabor
// @created: 08/08/2012
//

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>

namespace warthog::util
{

struct gm_header
{
	gm_header(uint32_t height, uint32_t width, const char* type)
	    : height_(height), width_(width), type_(type)
	{ }

	gm_header() = default;

	~gm_header() { }

	uint32_t height_;
	uint32_t width_;
	std::string type_;
};

} // namespace warthog::util

#endif // WARTHOG_UTIL_GM_PARSER_H
