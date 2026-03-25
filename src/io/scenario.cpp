#include <warthog/io/scenario.h>

#include <warthog/io/grid.h>
#include <warthog/util/scenario_manager.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>

namespace warthog::io
{

scenario_serialize::scenario_serialize()
    : m_line(static_cast<char*>(m_dyn_res.allocate(max_line_length + 2))),
      m_unget_line(&m_dyn_res), m_dist_strings(&m_dyn_res),
      m_dist_id(&m_dyn_res)
{
	m_line[0] = '\0';
}
scenario_serialize::~scenario_serialize() = default;

std::errc
scenario_serialize::open_read(std::istream* scenario)
{
	close();
	if(scenario != nullptr) { m_scenario_in = scenario; }
	else
	{
		auto stream   = std::make_unique<std::ifstream>(m_scenario_filename);
		m_scenario_in = stream.get();
		m_scenario_stream = std::move(stream);
	}
	if(!*m_scenario_in)
	{
		// bad stream
		m_scenario_in     = nullptr;
		m_scenario_stream = nullptr;
		return std::errc::io_error;
	}
	return std::errc{};
}

void
scenario_serialize::close()
{
	m_state        = serialize_state::init;
	m_version      = scenario_version::version1;
	m_scenario_in  = nullptr;
	m_scenario_out = nullptr;
	m_dist.reset();
	m_map_width  = 0;
	m_map_height = 0;
	m_query_at   = 0;
	m_line_num   = -1;
	m_line[0]    = '\0';
	m_unget_line.clear();
	m_dist_strings.clear();
	m_dist_id.clear();
	m_scenario_stream = nullptr;
}

std::pair<std::string_view, std::errc>
scenario_serialize::readline(std::istream* in)
{
	size_t len;
	if(!m_unget_line.empty())
	{
		// return last unreadline
		len = m_unget_line.length();
		if(len > max_line_length - 1)
		{
			return {{}, std::errc::invalid_argument};
		}
		std::memcpy(m_line, m_unget_line.c_str(), len + 1);
		m_unget_line.clear();
	}
	else
	{
		auto [s, err] = get_istream(in);
		if(err != std::errc{}) { return {{}, err}; }
		if(!s->getline(m_line, max_line_length))
		{
			if(s->eof())
				return {{}, {}};
			else
				return {{}, std::errc::io_error};
		}
		len = strlen(m_line);
	}
	m_line_num += 1;
	return {std::string_view(m_line, len), {}};
}
void
scenario_serialize::unreadline(std::string_view line)
{
	m_unget_line = line;
}

void
scenario_serialize::set_relative_map_filename(
    const std::filesystem::path& filename)
{
	m_map_filename = std::filesystem::proximate(filename, m_scenario_filename);
}

std::istream&
scenario_serialize::line_stream(std::string_view line)
{
	m_iss.clear();
	m_iss.str(std::string(line));
	m_iss.seekg(0);
	return m_iss;
}

std::errc
scenario_serialize::read_version(std::istream* in)
{
	if(m_state != serialize_state::init)
	{
		return std::errc::state_not_recoverable;
	}
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	std::string_view line;
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	auto& iss = line_stream(line);
	int version;
	if(!(iss >> m_token >> version))
	{
		m_state = serialize_state::error;
		return std::errc::io_error;
	}
	if(version < 1 || version > 2)
	{
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	m_version = version == 1 ? scenario_version::version1
	                         : scenario_version::version2;
	m_state   = serialize_state::header;
	return std::errc{};
}

std::errc
scenario_serialize::read_header(std::istream* in)
{
	if(!can_read(in) || m_state != serialize_state::header)
	{
		return std::errc::state_not_recoverable;
	}

	switch(m_version)
	{
	case scenario_version::version1:
		return read_header_v1(get_istream().first);
	case scenario_version::version2:
		return read_header_v2(get_istream().first);
	default:
		return std::errc::state_not_recoverable;
	}
}

std::errc
scenario_serialize::read_header_v1(std::istream* in)
{
	std::errc ec;
	bool s;
	if(in == nullptr)
	{
		std::tie(in, ec) = get_istream(in);
		if(ec != std::errc{})
		{
			m_state = serialize_state::error;
			return ec;
		}
	}
	// read first query line to get map
	scenario_query Q;
	std::tie(s, ec) = read_query_line_v1(Q);
	if(ec != std::errc{})
	{
		m_state = serialize_state::error;
		return ec;
	}
	if(s != final)
	{
		if(Q.width <= 0 || Q.width > GRID_MAX_SIZE || Q.height <= 0
		   || Q.height > GRID_MAX_SIZE)
		{
			m_state = serialize_state::error;
			return std::errc::invalid_argument;
		}
		set_relative_map_filename(Q.map);
		m_map_width  = Q.width;
		m_map_height = Q.height;
	}
	unreadline(m_line);
	m_state = serialize_state::query;
	return std::errc{};
}

std::errc
scenario_serialize::read_header_v2(std::istream* in)
{
	std::errc err;
	std::tie(in, err) = get_istream(in);
	if(err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	// read first query line to get map
	std::string_view line;
	// height
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	{
		auto& iss = line_stream(line);
		if(!(iss >> m_token >> m_map_height))
		{
			m_state = serialize_state::error;
			return std::errc::io_error;
		}
	}
	if(m_map_height < 1 || m_map_height > GRID_MAX_SIZE)
	{
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	// width
	std::tie(line, err) = readline(in);
	if(err != std::errc{})
	{
		m_state = serialize_state::error;
		return err;
	}
	{
		auto& iss = line_stream(line);
		if(!(iss >> m_token >> m_map_width))
		{
			m_state = serialize_state::error;
			return std::errc::io_error;
		}
	}
	if(m_map_width < 1 || m_map_width > GRID_MAX_SIZE)
	{
		m_state = serialize_state::error;
		return std::errc::invalid_argument;
	}
	// set_relative_map_filename(m_query.map);
	m_state = serialize_state::query;
	return std::errc{};
}

auto
scenario_serialize::read_query_line_v1(scenario_query& query, std::istream* in)
    -> std::pair<query_res, std::errc>
{
	assert(can_read(in));
	// move to start of read
	std::errc ec;
	std::string_view line;
	std::tie(in, ec) = get_istream(in);
	if(ec != std::errc{}) { return {invalid, std::errc::io_error}; }
	std::tie(line, ec) = readline(in);
	if(ec != std::errc{}) { return {invalid, std::errc::io_error}; }
	if(in->eof()) { return {final, std::errc{}}; }
	assert(!std::all_of(line.begin(), line.end(), [](char a) {
		return std::isspace((unsigned char)a);
	}));
	auto& iss = line_stream(line);
	if(!(iss >> query.bucket >> query.map >> query.width >> query.height
	     >> query.start_x >> query.start_y >> query.goal_x >> query.goal_y
	     >> query.dist[0]))
	{
		return {invalid, std::errc::io_error};
	}
	if(!std::isfinite(query.start_x) || !std::isfinite(query.start_y)
	   || !std::isfinite(query.goal_x) || !std::isfinite(query.goal_y)
	   || !std::isfinite(query.dist[0]))
	{
		return {invalid, std::errc::invalid_argument};
	}
	// perform checking of values, but do not return as error if fail
	if(query.width != m_map_width || query.height != m_map_height)
	{
		return {invalid, std::errc{}};
	}
	if(m_force_int)
	{
		// check integer bounds
		if(rint(query.start_x) != query.start_x
		   || rint(query.start_y) != query.start_y
		   || rint(query.goal_x) != query.goal_x
		   || rint(query.goal_y) != query.goal_y)
		{
			return {invalid, std::errc{}};
		}
		if(query.start_x < 0 || query.start_y >= m_map_width
		   || query.goal_x < 0 || query.goal_y >= m_map_height)
		{
			return {invalid, std::errc{}};
		}
	}
	else
	{
		if(query.start_x < 0 || query.start_y > m_map_width || query.goal_x < 0
		   || query.goal_y > m_map_height)
		{
			return {invalid, std::errc{}};
		}
	}
	return {valid, std::errc{}};
}

auto
scenario_serialize::read_query_line_v2(scenario_query& query, std::istream* in)
    -> std::pair<query_res, std::errc>
{
	assert(can_read(in));
	int width, height;
	double d;
	query.dist.fill(-1.0);
	if(!(m_iss >> query.bucket >> query.start_x >> query.start_y
	     >> query.goal_x >> query.goal_y))
	{
		return {invalid, std::errc::io_error};
	}
	for(auto i : m_dist_id)
	{
		if(!(m_iss >> d)) { return {invalid, std::errc::io_error}; }
		if(!std::isfinite(d))
		{
			return {invalid, std::errc::invalid_argument};
		}
		if(i >= 0) { query.dist[i] = d; }
	}
	m_iss >> std::ws;
	if(m_iss.eof()) { return {invalid, std::errc::invalid_argument}; }
	return {valid, std::errc{}};
}

auto
scenario_serialize::read_patch_line_v2(scenario_patch& patch, std::istream* in)
    -> std::pair<query_res, std::errc>
{
	assert(can_read());
	int width, height;
	double d;
	if(!(m_iss >> patch.bucket >> patch.patch_id >> patch.loc_x
	     >> patch.loc_y))
	{
		return {invalid, std::errc::io_error};
	}
	if(patch.loc_x >= m_map_width || patch.loc_y > m_map_height)
	{
		return {invalid, std::errc::invalid_argument};
	}

	m_iss >> std::ws;
	if(m_iss.eof()) { return {invalid, std::errc::invalid_argument}; }
	return {valid, std::errc{}};
}

} // namespace warthog::io
