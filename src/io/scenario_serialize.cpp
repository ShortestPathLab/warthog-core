#include <warthog/io/scenario_serialize.h>

#include <warthog/io/bittable_serialize.h>

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>

namespace warthog::io
{

scenario_serialize::scenario_serialize()
    : m_dyn_res(1024), m_string_res(256), m_cost_strings(&m_dyn_res),
      m_cost_type(&m_dyn_res), m_cost_value(&m_dyn_res)
{ }
scenario_serialize::~scenario_serialize() = default;

void
scenario_serialize::close()
{
	serialize_base::close();
	m_state      = serialize_state::INIT;
	m_version    = scenario_version::UNKNOWN;
	m_map_width  = 0;
	m_map_height = 0;
	m_inst_at    = 0;
	m_cost_strings.clear();
	m_cost_type.clear();
	m_cost_value.clear();
}

void
scenario_serialize::set_relative_map_filename(
    const std::filesystem::path& filename)
{
	m_map_filename
	    = std::filesystem::proximate(filename, get_scenario_filename());
}

int
scenario_serialize::last_command_type() const
{
	if(m_command_type == "Q") return CMD_INST;
	if(m_command_type == "P") return CMD_PATCH;
	return CMD_UNKNOWN;
}

std::expected<void, std::errc>
scenario_serialize::read_version(std::istream* in)
{
	if(m_state != serialize_state::INIT)
		return std::unexpected(std::errc::state_not_recoverable);
	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	std::string_view line;
	std::string_view token;

	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	parser par(line);
	int version;
	if(!par.next(token).next(version).eof())
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(par.error());
	}
	if(version < 1 || version > 2)
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(std::errc::invalid_argument);
	}
	m_version = version == 1 ? scenario_version::VERSION_1
	                         : scenario_version::VERSION_2;
	m_state   = serialize_state::VERSION;
	return {};
}

std::expected<void, std::errc>
scenario_serialize::read_header(std::istream* in)
{
	if(!can_read(in) || m_state != serialize_state::VERSION)
		return std::unexpected(std::errc::state_not_recoverable);

	switch(m_version)
	{
	case scenario_version::VERSION_1:
		return read_header_v1(in);
	case scenario_version::VERSION_2:
		return read_header_v2(in);
	default:
		return std::unexpected(std::errc::state_not_recoverable);
	}
}

std::expected<void, std::errc>
scenario_serialize::read_header_v1(std::istream* in)
{
	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	// setup cost types
	m_cost_strings.resize(1);
	m_cost_type.resize(1);
	m_cost_value.resize(1);
	m_cost_strings[0] = get_cost_str(cost_type::G_8C_NCC);
	m_cost_type[0]    = cost_type::G_8C_NCC;
	m_cost_value[0]   = -1;

	// read first inst line to get map
	auto line = readline(in);
	if(!line)
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(line.error());
	}
	parser par(*line);
	std::string_view map;
	if(!par.ignore().next(map).next(m_map_width).next(m_map_height))
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(par.error());
	}
	if(m_map_height < 1 || m_map_height > GRID_MAX_SIZE)
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(std::errc::invalid_argument);
	}
	if(m_map_width <= 0 || m_map_width > GRID_MAX_SIZE || m_map_height <= 0
	   || m_map_height > GRID_MAX_SIZE)
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(std::errc::invalid_argument);
	}
	set_relative_map_filename(map);
	unreadline(*line);
	m_state = serialize_state::COMMAND;
	return {};
}

std::expected<void, std::errc>
scenario_serialize::read_header_v2(std::istream* in)
{
	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	// read first inst line to get map
	std::string_view line;
	std::string_view token;

	// height
	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	{
		parser par(line);
		if(!par.next(token).next(m_map_height).eof())
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
		if(token != "height" || m_map_height < 1
		   || m_map_height > GRID_MAX_SIZE)
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(std::errc::invalid_argument);
		}
	}

	// width
	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	{
		parser par(line);
		if(!par.next(token).next(m_map_width).eof())
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
		if(token != "width" || m_map_width < 1 || m_map_width > GRID_MAX_SIZE)
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(std::errc::invalid_argument);
		}
	}

	// cost
	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	{
		parser par(line);
		int cost_count;
		if(!par.next(token).next(cost_count))
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
		if(token != "cost" || cost_count < 0 || cost_count > 128)
		{
			// limit cost_count to be within reason
			m_state = serialize_state::ERROR;
			return std::unexpected(std::errc::invalid_argument);
		}
		m_cost_strings.resize(cost_count);
		m_cost_type.resize(cost_count);
		m_cost_value.resize(cost_count);
		for(int i = 0; i < cost_count; ++i)
		{
			if(!par.next(token))
			{
				m_state = serialize_state::ERROR;
				return std::unexpected(par.error());
			}
			std::string_view ctype = copy_string(token);
			m_cost_strings[i]      = ctype;
			m_cost_type[i]         = get_cost_type(ctype);
			m_cost_value[i]        = -1;
		}
		// end of line
		if(!par.eof())
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
	}

	// get map (patch) filename
	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	{
		parser par(line);
		std::string fname;
		if(!par.next(token).next_q(fname).eof())
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
		m_map_filename = std::move(fname);
		if(token != "patch")
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(std::errc::invalid_argument);
		}
	}

	// read in "commands"
	if(auto r = readline(in); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	{
		parser par(line);
		if(!par.next(token).eof())
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(par.error());
		}
		if(token != "commands")
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(std::errc::invalid_argument);
		}
	}

	// set_relative_map_filename(m_inst.map);
	m_state = serialize_state::COMMAND;
	return {};
}

std::expected<int, std::errc>
scenario_serialize::next_command_type(std::istream* in)
{
	if(!can_read(in) || m_state != serialize_state::COMMAND)
		return std::unexpected(std::errc::state_not_recoverable);
	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}

	switch(m_version)
	{
	case scenario_version::VERSION_1:
	{
		auto line = readline(in);
		if(!line)
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(line.error());
		}
		if(istream_eof(in))
		{
			m_state = serialize_state::END;
			return FINAL;
		}
		unreadline(*line);
		return CMD_INST;
	}
	case scenario_version::VERSION_2:
	{
		auto line = readline(in);
		if(!line)
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(line.error());
		}
		if(istream_eof(in))
		{
			m_state = serialize_state::END;
			return FINAL;
		}
		parser par(*line);
		if(!par.next(m_command_type)) return std::unexpected(par.error());
		auto cmd_type = last_command_type();
		unreadline(*line);
		return cmd_type;
	}
	default:
		return std::unexpected(std::errc::state_not_recoverable);
	}
}

std::expected<int, std::errc>
scenario_serialize::skip_commands(int count, std::istream* in)
{
	if(m_state == serialize_state::END) { return {}; }
	if(!can_read(in) || m_state != serialize_state::COMMAND)
		return std::unexpected(std::errc::state_not_recoverable);

	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	int skipped = 0;
	while(count > 0)
	{
		--count;
		if(auto r = readline(in, true); !r)
		{
			m_state = serialize_state::ERROR;
			return std::unexpected(r.error());
		}
		if(istream_eof(in))
		{
			m_state = serialize_state::END;
			break;
		}
		++skipped;
	}
	return skipped;
}

std::expected<int, std::errc>
scenario_serialize::read_instance_line(
    scenario_instance& inst, std::istream* in)
{
	if(m_state == serialize_state::END) return FINAL;
	if(!can_read(in) || m_state != serialize_state::COMMAND)
		return std::unexpected(std::errc::state_not_recoverable);

	switch(m_version)
	{
	case scenario_version::VERSION_1:
		return read_instance_line_v1(inst, in);
	case scenario_version::VERSION_2:
		return read_instance_line_v2(inst, in);
	default:
		return std::unexpected(std::errc::state_not_recoverable);
	}
}

std::expected<int, std::errc>
scenario_serialize::read_instance_line_v1(
    scenario_instance& inst, std::istream* in)
{
	assert(can_read(in));
	// move to start of read
	std::string_view line;
	std::string_view token;

	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(auto r = readline(in, true); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(istream_eof(in))
	{
		m_state = serialize_state::END;
		return FINAL;
	}
	parser par(line);
	if(!par.next(inst.bucket)
	        .next(token)
	        .next(inst.width)
	        .next(inst.height)
	        .next(inst.start_x)
	        .next(inst.start_y)
	        .next(inst.goal_x)
	        .next(inst.goal_y)
	        .next(m_cost_value.at(0))
	        .eof())
	{
		// state not set to error, up to user
		return std::unexpected(par.error());
	}
	inst.map  = token;
	inst.cost = m_cost_value;
	if(!std::isfinite(inst.start_x) || !std::isfinite(inst.start_y)
	   || !std::isfinite(inst.goal_x) || !std::isfinite(inst.goal_y)
	   || !std::isfinite(inst.cost[0]))
		return INVALID;
	// perform checking of values, but do not return as error if fail
	if(inst.width != m_map_width || inst.height != m_map_height)
		return INVALID;
	if(m_force_int)
	{
		// check integer bounds
		if(rint(inst.start_x) != inst.start_x
		   || rint(inst.start_y) != inst.start_y
		   || rint(inst.goal_x) != inst.goal_x
		   || rint(inst.goal_y) != inst.goal_y)
			return INVALID;
		if(inst.start_x < 0 || inst.start_x >= m_map_width || inst.goal_x < 0
		   || inst.goal_y >= m_map_height)
			return INVALID;
	}
	else
	{
		if(inst.start_x < 0 || inst.start_x > m_map_width || inst.goal_x < 0
		   || inst.goal_y > m_map_height)
			return INVALID;
	}
	return VALID;
}

std::expected<int, std::errc>
scenario_serialize::read_instance_line_v2(
    scenario_instance& inst, std::istream* in)
{
	assert(can_read(in));
	// move to start of read
	std::string_view line;
	std::string_view token;

	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(auto r = readline(in, true); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(istream_eof(in))
	{
		m_state = serialize_state::END;
		return FINAL;
	}

	parser par(line);
	if(!par.next(m_command_type)) return std::unexpected(par.error());
	auto cmd_type = last_command_type();
	if(cmd_type != CMD_INST)
	{
		unreadline(line);
		return cmd_type;
	}
	if(!par.next(inst.bucket)
	        .next(inst.start_x)
	        .next(inst.start_y)
	        .next(inst.goal_x)
	        .next(inst.goal_y))
		return std::unexpected(par.error());
	if(!std::isfinite(inst.start_x) || !std::isfinite(inst.start_y)
	   || !std::isfinite(inst.goal_x) || !std::isfinite(inst.goal_y))
		return std::unexpected(std::errc::invalid_argument);
	inst.width = inst.height = 0;
	for(auto& cost : m_cost_value)
	{
		if(!par.next(cost)) return std::unexpected(par.error());
		if(!std::isfinite(cost))
			return std::unexpected(std::errc::invalid_argument);
	}
	inst.cost = m_cost_value;
	if(!par.eof())
	{
		// unexpected number of paramters
		return std::unexpected(par.error());
	}
	if(m_force_int)
	{
		// check integer bounds
		if(rint(inst.start_x) != inst.start_x
		   || rint(inst.start_y) != inst.start_y
		   || rint(inst.goal_x) != inst.goal_x
		   || rint(inst.goal_y) != inst.goal_y)
			return INVALID;
		if(inst.start_x < 0 || inst.start_x >= m_map_width || inst.goal_x < 0
		   || inst.goal_y >= m_map_height)
			return INVALID;
	}
	else
	{
		if(inst.start_x < 0 || inst.start_x > m_map_width || inst.goal_x < 0
		   || inst.goal_y > m_map_height)
		{
			return INVALID;
		}
	}
	return VALID;
}

std::expected<int, std::errc>
scenario_serialize::read_patch_line(scenario_patch& patch, std::istream* in)
{
	if(!can_read(in) || m_state != serialize_state::COMMAND)
	{
		return std::unexpected(std::errc::state_not_recoverable);
	}

	switch(m_version)
	{
	case scenario_version::VERSION_2:
		return read_patch_line_v2(patch, in);
	default:
		return std::unexpected(std::errc::state_not_recoverable);
	}
}

auto
scenario_serialize::read_patch_line_v2(scenario_patch& patch, std::istream* in)
    -> std::expected<int, std::errc>
{
	assert(can_read(in));
	// move to start of read
	std::string_view line;
	std::string_view token;

	if(auto r = get_istream(in); r) { in = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(auto r = readline(in, true); r) { line = *r; }
	else
	{
		m_state = serialize_state::ERROR;
		return std::unexpected(r.error());
	}
	if(istream_eof(in))
	{
		m_state = serialize_state::END;
		return FINAL;
	}

	parser par(line);
	if(!par.next(token)) return std::unexpected(par.error());
	auto cmd_type = last_command_type();
	if(cmd_type != CMD_PATCH)
	{
		unreadline(line);
		return cmd_type;
	}

	if(!par.next(patch.bucket)
	        .next(patch.patch_id)
	        .next(patch.loc_x)
	        .next(patch.loc_y)
	        .eof())
		return std::unexpected(par.error());
	if(patch.loc_x >= m_map_width || patch.loc_y >= m_map_height)
		return INVALID;

	return VALID;
}

std::string_view
scenario_serialize::copy_string(std::string_view str)
{
	size_t len = str.size();
	if(len == 0) return std::string_view();
	char* data = static_cast<char*>(m_string_res.allocate(len + 1, 8));
	std::memcpy(data, str.data(), len);
	data[len] = '\0';
	return std::string_view(data, len);
}

} // namespace warthog::io
