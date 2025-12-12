#include <warthog/io/scenario.h>

#include <iomanip>
#include <cstring>

namespace warthog::io
{

scenario_serialize::scenario_serialize() : m_line(max_line_length + 2, '\0', &m_dyn_res),
	m_dist_strings(&m_dyn_res),
	m_dist_id(&m_dyn_res)
{ }
scenario_serialize::~scenario_serialize() = default;

std::pair<std::string_view, std::errc> scenario_serialize::readline(std::istream* in)
{
	auto [s, err] = get_instream(in);
	if (err != std::errc{})
	{
		return {{}, err};
	}
	s->getline(m_line.data(), max_line_length);
}

std::errc scenario_serialize::read_version(std::istream* in)
{
	auto [s, err] = get_instream(in);
	if (err != std::errc{})
	{
		return err;
	}
	
}

} // namespace warthog::io
