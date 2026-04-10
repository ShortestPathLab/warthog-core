#include <warthog/io/serialize_base.h>

#include <string>
#include <fstream>
#include <iomanip>
#include <cstring>

namespace warthog::io
{

serialize_base::serialize_base() : m_line_data(std::make_unique<char[]>(max_line_length))
{ }
serialize_base::~serialize_base() = default;

std::errc
serialize_base::open_read(std::istream* is)
{
	close();
	if(is != nullptr) { m_stream_in = is; }
	else
	{
		auto stream   = std::make_unique<std::ifstream>(get_filename());
		m_stream_in = stream.get();
		m_stream = std::move(stream);
	}
	if(!*m_stream_in)
	{
		// bad stream
		m_stream_in     = nullptr;
		m_stream = nullptr;
		return std::errc::io_error;
	}
	return std::errc{};
}

std::errc
serialize_base::open_write(std::ostream* os)
{
	close();
	if(os != nullptr) { m_stream_out = os; }
	else
	{
		auto stream   = std::make_unique<std::ofstream>(get_filename());
		m_stream_out = stream.get();
		m_stream = std::move(stream);
	}
	if(!*m_stream_out)
	{
		// bad stream
		m_stream_out     = nullptr;
		m_stream = nullptr;
		return std::errc::io_error;
	}
	return std::errc{};
}

void
serialize_base::close()
{
	m_line_num   = -1;
	m_line = std::string_view();
	m_unget_line.clear();
	m_stream = nullptr;
	m_stream_in = nullptr;
	m_stream_out = nullptr;
}

std::pair<std::string_view, std::errc>
serialize_base::readline(std::istream* in)
{
	size_t len;
	if(len = m_unget_line.size(); len != 0)
	{
		// return last unreadline
		if(len > max_line_length - 1)
		{
			return {{}, std::errc::invalid_argument};
		}
		if (len == 1 && m_unget_line[0] == '\0') {
			// empty line
			m_line = std::string_view();
		} else {
			// copy line
			std::memcpy(m_line_data.get(), m_unget_line.c_str(), len+1);
			m_line = std::string_view(m_line_data.get(), len);
		}
		m_unget_line.clear();
	}
	else
	{
		auto [s, err] = get_istream(in);
		if(err != std::errc{}) { return {{}, err}; }
		if(!s->getline(m_line_data.get(), max_line_length))
		{
			return {{}, std::errc::io_error};
		}
		// get length, if eof is set, is last line and no delimiter was extracted
		len = static_cast<size_t>(s->gcount()) - static_cast<std::size_t>(s->eof());
		m_line_num += 1;
	}
	return {std::string_view(m_line_data.get(), len), {}};
}
void
serialize_base::unreadline(std::string_view line)
{
	if (!line.empty()) [[likely]] {
		m_unget_line = line;
	} else {
		m_unget_line = '\0';
	}
}

std::istream&
serialize_base::line_stream(std::string_view line)
{
	m_iss.clear();
	m_iss.str(std::string(line));
	m_iss.seekg(0);
	return m_iss;
}
bool
serialize_base::line_stream_eof()
{
	if (m_iss && !m_iss.eof()) {
		m_iss >> std::ws;
	}
	return m_iss && m_iss.eof();
}

} // namespace warthog::io
