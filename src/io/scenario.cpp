#include <warthog/io/scenario.h>

#include <iomanip>
#include <cstring>
#include <cmath>
#include <warthog/util/scenario_manager.h>

namespace warthog::io
{

scenario_serialize::scenario_serialize() : m_line(max_line_length + 2, '\0', &m_dyn_res),
	m_dist_strings(&m_dyn_res),
	m_dist_id(&m_dyn_res)
{ }
scenario_serialize::~scenario_serialize() = default;

std::pair<std::string_view, std::errc> scenario_serialize::readline(std::istream* in)
{
	auto [s, err] = get_istream(in);
	if (err != std::errc{})
	{
		return {{}, err};
	}
	s->getline(m_line.data(), max_line_length);
	return {m_line, {}};
}

std::errc scenario_serialize::read_version(std::istream* in)
{
	if (m_state != serialize_state::init)
	{
		return std::errc::state_not_recoverable;
	}
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if (err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	std::string_view line;
	std::tie(line, err) = readline(in);
	if (err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	m_iss.str(std::string(line));
	int version;
	if (!(*in >> m_token >> version)) {
		m_state = serialize_state::error;
		return std::errc::io_error;
	}
	if (version < 1 || version > 2) {
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	m_version = version == 1 ? scenario_version::version1 : scenario_version::version2;
	m_state = serialize_state::header;
	return std::errc{};
}

std::errc scenario_serialize::read_header(std::istream* in)
{
	if (!can_read(in) || m_state != serialize_state::header)
	{
		return std::errc::state_not_recoverable;
	}

	switch (m_version)
	{
	case scenario_version::version1:
		return read_header_v1(get_istream().first);
	case scenario_version::version2:
		return read_header_v2(get_istream().first);
	default:
		return std::errc::state_not_recoverable;
	}
}

std::errc scenario_serialize::read_header_v1(std::istream* in)
{
	std::errc ec;
	bool s;
	if (in == nullptr) {
		std::tie(in, ec) = get_istream(in);
		if (ec != std::errc{})
		{
			m_state = serialize_state::error;
			return ec;
		}
	}
	// read first query line to get map
	std::tie(s, ec) = read_query_line_v1(m_query);
	if (ec != std::errc{})
	{
		m_state = serialize_state::error;
		return ec;
	}
	if (s)
	{
		set_relative_map_filename(m_query.map);
	}
	m_state = serialize_state::query;
	return std::errc{};
}

std::errc scenario_serialize::read_header_v2(std::istream* in)
{
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if (err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	// read first query line to get map
	std::string_view line;
	// height
	std::tie(line, err) = readline(in);
	if (err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	m_iss.str(std::string(line));
	if (!(m_iss >> m_token >> m_map_height))
	{
		m_state = serialize_state::error;
		return std::errc::io_error;
	}
	if (m_map_height < 1 || m_map_height > max_dimension)
	{
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	// width
	std::tie(line, err) = readline(in);
	if (err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	m_iss.str(std::string(line));
	if (!(m_iss >> m_token >> m_map_width))
	{
		m_state = serialize_state::error;
		return std::errc::io_error;
	}
	if (m_map_width < 1 || m_map_width > max_dimension)
	{
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	set_relative_map_filename(m_query.map);
	m_state = serialize_state::query;
	return std::errc{};
}

std::pair<bool,std::errc> scenario_serialize::read_query_line_v1(scenario_query& query)
{
	assert(can_read());
	int width, height;
	// move to start of read
	auto [iss,ec] = get_istream();
	if (ec != std::errc{})
	{
		return {false, std::errc::io_error};
	}
	auto [line,ec] = readline();
	if (!(m_iss >> std::ws))
	{
		return {false, std::errc::io_error};
	}
	if (m_iss.eof())
	{
		return {false, std::errc{}};
	}
	if (!(m_iss >> query.bucket >> query.map >> width >> height >> query.start_x >> query.start_y >> query.goal_x >> query.goal_y >> query.dist[0]))
	{
		return {false, std::errc::io_error};
	}
	if (!std::isfinite(query.start_x) || !std::isfinite(query.start_y) || !std::isfinite(query.goal_x) || !std::isfinite(query.goal_y) || !std::isfinite(query.dist[0]))
	{
		return {false, std::errc::invalid_argument};
	}
	return {false, std::errc{}};
}

std::pair<bool,std::errc> scenario_serialize::read_query_line_v2(scenario_query& query)
{
	assert(can_read());
	int width, height;
	double d;
	query.dist.fill(-1.0);
	if (!(m_iss >> query.bucket >> query.start_x >> query.start_y >> query.goal_x >> query.goal_y))
	{
		return {false, std::errc::io_error};
	}
	for (auto i : m_dist_id)
	{
		if (!(m_iss >> d))
		{
			return {false, std::errc::io_error};
		}
		if (!std::isfinite(d))
		{
			return {false, std::errc::invalid_argument};
		}
		if (i >= 0) {
			query.dist[i] = d;
		}
	}
	m_iss >> std::ws;
	if (m_iss.eof())
	{
		return {false, std::errc::invalid_argument};
	}
	return {false, std::errc{}};
}

std::pair<bool,std::errc> scenario_serialize::read_patch_line_v2(scenario_patch& patch)
{
	assert(can_read());
	int width, height;
	double d;
	if (!(m_iss >> patch.bucket >> patch.patch_id >> patch.loc_x >> patch.loc_y))
	{
		return {false, std::errc::io_error};
	}
	if (patch.loc_x >= m_map_width || patch.loc_y > m_map_height)
	{
		return {false, std::errc::invalid_argument};
	}

	m_iss >> std::ws;
	if (m_iss.eof())
	{
		return {false, std::errc::invalid_argument};
	}
	return {false, std::errc{}};
}

} // namespace warthog::io
