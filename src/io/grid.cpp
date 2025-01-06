#include <warthog/io/grid.h>
#include <iomanip>
#include <cstring>

namespace warthog::io
{

bool bittable_serialize::read_header(std::istream& in)
{
	char buffer[32];
	auto read_key = [&]() {
		return static_cast<bool>(in >> std::setw(32) >> buffer);
	};

	if (!read_key())
		return false;
	bittable_type detected_type = bittable_type::NONE;
	if (std::strcmp(buffer, "type") == 0) {
		if (!read_key())
			return false;
		if (std::strcmp(buffer, "octile") == 0)
			detected_type = bittable_type::OCTILE;
		else if (std::strcmp(buffer, "patch") == 0)
			detected_type = bittable_type::PATCH;
		else
			detected_type = bittable_type::OTHER;

		if (!read_key())
			return false;
	}
		
	// read patch header before map header
	if (detected_type == bittable_type::PATCH) {
		if (std::strcmp(buffer, "patches") != 0)
			return false;
		in >> m_patch_count;
		if (!read_key())
			return false;
	}

	if (std::strcmp(buffer, "height") != 0)
		return false;
	if (!(in >> m_dim.height))
		return false;
	if (!read_key())
		return false;
	if (std::strcmp(buffer, "width") != 0)
		return false;
	if (!(in >> m_dim.width))
		return false;
	m_type = detected_type;
	if (!read_key() || std::strcmp(buffer, "map") != 0)
		return false;
	return true;
}

} // namespace warthog::io
