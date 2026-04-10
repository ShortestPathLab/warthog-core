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
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	bittable_type detected_type = bittable_type::NONE;
	{
	auto& iss = line_stream(line);
	if (!(iss >> m_token) || m_token != "type") {
		return std::errc::io_error;
	}
	if (!(iss >> m_token) || !line_stream_eof()) {
		return std::errc::io_error;
	}
	}

	if(m_token == "octile")
		detected_type = bittable_type::OCTILE;
	else if(m_token == "patch")
		detected_type = bittable_type::PATCH;
	else
		detected_type = bittable_type::OTHER;
	m_type = detected_type;

	// read patch header before map header
	if(detected_type == bittable_type::PATCH)
	{
		std::tie(line, err) = readline(in);
		if(err != std::errc{})
		{
			return err;
		}
		auto& iss = line_stream(line);
		if (!(iss >> m_token >> m_patch_count) || !line_stream_eof()) {
			return std::errc::io_error;
		}
		if (m_token != "patches") {
			return std::errc::argument_out_of_domain;
		}
	}

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

	// read height
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		return err;
	}
	{
		auto& iss = line_stream(line);
		if (!(iss >> m_token >> m_dim.height) || !line_stream_eof())
		{
			return std::errc::io_error;
		}
		if (m_token != "height" || m_dim.height > GRID_DIMENSION_MAX) {
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
		auto& iss = line_stream(line);
		if (!(iss >> m_token >> m_dim.width) || !line_stream_eof())
		{
			return std::errc::io_error;
		}
		if (m_token != "width" || m_dim.width > GRID_DIMENSION_MAX) {
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
		auto& iss = line_stream(line);
		if (!(iss >> m_token) || !line_stream_eof())
		{
			return std::errc::io_error;
		}
		if (m_token != "map") {
			return std::errc::argument_out_of_domain;
		}
	}

	return std::errc{};
}

std::errc
bittable_serialize::read_grid_raw(
    std::vector<char>& raw_data, std::istream* in)
{
	const memory::bittable_dimension read_dim = m_dim;
	raw_data.resize(read_dim.width * read_dim.height);
	char* data_at = raw_data.data();

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
		auto& iss = line_stream(line);
		if (!(iss >> m_token) || !line_stream_eof())
		{
			return std::errc::io_error;
		}
		if (m_token.size() != read_dim.width)
			return std::errc::argument_out_of_domain;
		// copy row to table
		std::memcpy(data_at, m_token.data(), read_dim.width);
		data_at += read_dim.width;
	}
	return std::errc{};
}

} // namespace warthog::io
