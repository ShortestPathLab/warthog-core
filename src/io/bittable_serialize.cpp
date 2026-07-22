#include <warthog/io/bittable_serialize.h>

#include <cstring>
#include <iomanip>

namespace warthog::io
{

bittable_serialize::bittable_serialize()
{
	// increase max line length for larger grids
	set_max_line_length(20 << 10);
}

std::expected<void, std::errc>
bittable_serialize::read_header(std::istream* in)
{
	if(auto r = get_istream(in); r)
		in = *r;
	else
		return std::unexpected(r.error());

	std::string_view line;
	std::string_view token;
	if(auto r = readline(in); r)
		line = *r;
	else
		return std::unexpected(r.error());
	bittable_type detected_type = bittable_type::NONE;
	{
		parser par(line);
		if(!par.next(token) || token != "type")
			return std::unexpected(std::errc::io_error);
		if(!par.next(token).eof()) return std::unexpected(std::errc::io_error);
	}

	if(token == "octile")
		detected_type = bittable_type::OCTILE;
	else if(token == "patch")
		detected_type = bittable_type::PATCH;
	else
		detected_type = bittable_type::OTHER;
	m_type = detected_type;

	// read patch header before map header
	if(detected_type == bittable_type::OCTILE) { m_patch_amount = 1; }
	else if(detected_type == bittable_type::PATCH)
	{
		if(auto r = readline(in); r)
			line = *r;
		else
			return std::unexpected(r.error());
		parser par(line);
		if(!par.next(token).next(m_patch_amount).eof())
			return std::unexpected(std::errc::io_error);
		if(token != "patches" || m_patch_amount > PATCH_COUNT_LIMIT)
		{
			m_patch_amount = 0;
			return std::unexpected(std::errc::argument_out_of_domain);
		}
	}
	m_patch_count = 0;
	m_patch_id    = 0;

	return {};
}

std::expected<void, std::errc>
bittable_serialize::read_grid_header(std::istream* in)
{
	std::errc err;
	if(auto r = get_istream(in); r)
		in = *r;
	else
		return std::unexpected(r.error());
	std::string_view line;
	std::string_view token;

	m_patch_count += 1;

	// read height
	if(auto r = readline(in); r)
		line = *r;
	else
		return std::unexpected(r.error());
	if(m_type == bittable_type::PATCH)
	{
		// get patch number
		parser par(line);
		if(!par.next(token).next(m_patch_id).eof())
			return std::unexpected(std::errc::io_error);
		if(token != "patch")
			return std::unexpected(std::errc::argument_out_of_domain);

		if(auto r = readline(in); r)
			line = *r;
		else
			return std::unexpected(r.error());
	}
	{
		parser par(line);
		if(!par.next(token).next(m_dim.height).eof())
			return std::unexpected(std::errc::io_error);
		if(token != "height" || m_dim.height > GRID_DIMENSION_MAX)
			return std::unexpected(std::errc::argument_out_of_domain);
	}

	// read width
	if(auto r = readline(in); r)
		line = *r;
	else
		return std::unexpected(r.error());
	{
		parser par(line);
		if(!par.next(token).next(m_dim.width).eof())
			return std::unexpected(std::errc::io_error);
		if(token != "width" || m_dim.width > GRID_DIMENSION_MAX)
			return std::unexpected(std::errc::argument_out_of_domain);
	}

	// read "map"
	if(auto r = readline(in); r)
		line = *r;
	else
		return std::unexpected(r.error());
	{
		parser par(line);
		if(!par.next(token).eof()) return std::unexpected(std::errc::io_error);
		if(token != "map")
			return std::unexpected(std::errc::argument_out_of_domain);
	}

	return {};
}

std::expected<void, std::errc>
bittable_serialize::read_grid_raw(std::span<char> buffer, std::istream* in)
{
	const memory::bittable_dimension read_dim = m_dim;
	if(buffer.size() < (uint64_t)read_dim.width * (uint64_t)read_dim.height)
		return std::unexpected(std::errc::result_out_of_range);
	char* data_at = buffer.data();
	std::string_view line;
	std::string_view token;

	if(auto r = get_istream(in); r)
		in = *r;
	else
		return std::unexpected(r.error());
	for(uint32_t y = 0; y < read_dim.height; ++y)
	{
		// read row
		if(auto r = readline(in); r)
			line = *r;
		else
			return std::unexpected(r.error());
		parser par(line);
		if(!par.next(token).eof()) return std::unexpected(std::errc::io_error);
		if(token.size() != read_dim.width)
			return std::unexpected(std::errc::argument_out_of_domain);
		// copy row to table
		token.copy(data_at, token.size());
		data_at += read_dim.width;
	}
	return {};
}

std::expected<void, std::errc>
bittable_serialize::write_header(std::ostream* out)
{
	using namespace std::string_view_literals;
	if(auto r = get_ostream(out); r)
		out = *r;
	else
		return std::unexpected(r.error());

	m_patch_auto_pos = 0;
	m_patch_count    = 0;
	m_patch_id       = 0;
	if(m_type == bittable_type::OCTILE)
	{
		// expect single map, user does not need to provide this info
		if(!(*out << "type octile\n"))
			return std::unexpected(std::errc::io_error);
		m_patch_amount = 1;
	}
	else if(m_type == bittable_type::PATCH)
	{
		// expect variable number of patches
		if(!(*out << "type patch\npatches "))
			return std::unexpected(std::errc::io_error);
		if(m_patch_amount == patch_auto)
		{
			// not known in advance, out must be seekable to work
			m_patch_auto_pos = out->tellp();
			if(m_patch_auto_pos < 0 || !*out)
				return std::unexpected(std::errc::invalid_seek);
			// output 9 spaces, replace at end
			*out << "         "sv;
		}
		else
		{
			// set number of patches
			*out << m_patch_amount;
		}
	}
	else { return std::unexpected(std::errc::invalid_argument); }
	// check after previous write
	if(!*out) return std::unexpected(std::errc::io_error);

	m_patch_count += 1;
	return {};
}

std::expected<void, std::errc>
bittable_serialize::write_grid_raw(std::span<char> buffer, std::ostream* out)
{
	const memory::bittable_dimension write_dim = m_dim;
	if(buffer.size() < (uint64_t)write_dim.width * (uint64_t)write_dim.height)
		return std::unexpected(std::errc::result_out_of_range);
	char* data_at = buffer.data();

	if(auto r = get_ostream(out); r)
		out = *r;
	else
		return std::unexpected(r.error());

	if(m_type == bittable_type::PATCH)
	{
		if(!(*out << "patch " << m_patch_id++ << '\n'))
			return std::unexpected(std::errc::io_error);
	}
	if(!(*out << "height " << write_dim.height << "\nwidth " << write_dim.width
	          << "\nmap\n"))
		return std::unexpected(std::errc::io_error);

	for(uint32_t y = 0; y < write_dim.height; ++y)
	{
		// read row
		out->write(data_at, write_dim.width);
		out->put('\n');
		if(!*out) return std::unexpected(std::errc::io_error);
		data_at += write_dim.width;
	}

	m_patch_count += 1;
	return {};
}

std::expected<void, std::errc>
bittable_serialize::write_end(std::ostream* out)
{
	if(auto r = get_ostream(out); r)
		out = *r;
	else
		return std::unexpected(r.error());
	if(!*out) return std::unexpected(std::errc::io_error);

	if(m_patch_amount == patch_auto)
	{
		// auto, go and write m_patch_count
		if(!out->seekp(m_patch_auto_pos))
			return std::unexpected(std::errc::invalid_seek);
		if(!*out << m_patch_count) return std::unexpected(std::errc::io_error);
	}
	else
	{
		if(m_patch_count != m_patch_amount)
			return std::unexpected(std::errc::result_out_of_range);
	}

	return {};
}

} // namespace warthog::io
