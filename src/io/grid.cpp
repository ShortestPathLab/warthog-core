#include <warthog/io/grid.h>

#include <cstring>
#include <iomanip>

namespace warthog::io
{

std::errc
bittable_serialize::read_header(std::istream* in)
{
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		return err;
	}
	std::string_view line;
	std::string_view token;

	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	bittable_type detected_type = bittable_type::NONE;
	{
	parser par(line);
	if (!par.next(token) || token != "type") {
		return std::errc::io_error;
	}
	if (!par.next(token).eof()) {
		return std::errc::io_error;
	}
	}

	if(token == "octile")
		detected_type = bittable_type::OCTILE;
	else if(token == "patch")
		detected_type = bittable_type::PATCH;
	else
		detected_type = bittable_type::OTHER;
	m_type = detected_type;

	// read patch header before map header
	if (detected_type == bittable_type::OCTILE) {
		m_patch_amount = 1;
	} else 
	if(detected_type == bittable_type::PATCH)
	{
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
		parser par(line);
		if (!par.next(token).next(m_patch_amount).eof()) {
			return std::errc::io_error;
		}
		if (token != "patches" || m_patch_amount > PATCH_COUNT_LIMIT) {
			m_patch_amount = 0;
			return std::errc::argument_out_of_domain;
		}
	}
	m_patch_count = 0;
	m_patch_id = 0;

	return std::errc{};
}

std::errc
bittable_serialize::read_grid_header(std::istream* in)
{
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		return err;
	}
	std::string_view line;
	std::string_view token;

	m_patch_count += 1;

	// read height
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	if (m_type == bittable_type::PATCH) {
		// get patch number
		parser par(line);
		if (!par.next(token).next(m_patch_id).eof())
		{
			return std::errc::io_error;
		}
		if (token != "patch") {
			return std::errc::argument_out_of_domain;
		}
		
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
	}
	{
		parser par(line);
		if (!par.next(token).next(m_dim.height).eof())
		{
			return std::errc::io_error;
		}
		if (token != "height" || m_dim.height > GRID_DIMENSION_MAX) {
			return std::errc::argument_out_of_domain;
		}
	}
	// read width
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	{
		parser par(line);
		if (!par.next(token).next(m_dim.width).eof())
		{
			return std::errc::io_error;
		}
		if (token != "width" || m_dim.width > GRID_DIMENSION_MAX) {
			return std::errc::argument_out_of_domain;
		}
	}
	// read "map"
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	{
		parser par(line);
		if (!par.next(token).eof())
		{
			return std::errc::io_error;
		}
		if (token != "map") {
			return std::errc::argument_out_of_domain;
		}
	}

	return std::errc{};
}

std::errc
bittable_serialize::read_grid_raw(
    std::span<char> buffer, std::istream* in)
{
	const memory::bittable_dimension read_dim = m_dim;
	if (buffer.size() < (uint64_t)read_dim.width * (uint64_t)read_dim.height)
	{
		return std::errc::result_out_of_range;
	}
	char* data_at = buffer.data();
	std::string_view token;

	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		return err;
	}
	std::string_view line;
	for(uint32_t y = 0; y < read_dim.height; ++y)
	{
		// read row
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
		parser par(line);
		if (!par.next(token).eof())
		{
			return std::errc::io_error;
		}
		if (token.size() != read_dim.width)
			return std::errc::argument_out_of_domain;
		// copy row to table
		token.copy(data_at, token.size());
		data_at += read_dim.width;
	}
	return std::errc{};
}

} // namespace warthog::io
