#include <warthog/io/serialize_base.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>

namespace warthog::io
{

serialize_base::serialize_base()
    : m_line_data(std::make_unique<char[]>(max_line_length))
{ }
serialize_base::~serialize_base() = default;

std::errc
serialize_base::open_read(std::istream* is)
{
	close();
	if(is != nullptr) { m_stream_in = is; }
	else
	{
		auto stream = std::make_unique<std::ifstream>(get_filename());
		m_stream_in = stream.get();
		m_stream    = std::move(stream);
	}
	if(!*m_stream_in)
	{
		// bad stream
		m_stream_in = nullptr;
		m_stream    = nullptr;
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
		auto stream  = std::make_unique<std::ofstream>(get_filename());
		m_stream_out = stream.get();
		m_stream     = std::move(stream);
	}
	if(!*m_stream_out)
	{
		// bad stream
		m_stream_out = nullptr;
		m_stream     = nullptr;
		return std::errc::io_error;
	}
	return std::errc{};
}

void
serialize_base::close()
{
	m_line_num = 0;
	m_line     = std::string_view();
	m_unget_line.clear();
	m_stream     = nullptr;
	m_stream_in  = nullptr;
	m_stream_out = nullptr;
}

bool
serialize_base::istream_eof(std::istream* in)
{
	auto [s, err] = get_istream(in);
	if(err != std::errc{}) return false;
	return s->eof();
}

std::pair<std::string_view, std::errc>
serialize_base::readline(std::istream* in, bool skip_blanks)
{
	size_t len;
	auto [s, err] = get_istream(in);
	while(true)
	{
		if(len = m_unget_line.size(); len != 0)
		{
			// return last unreadline
			if(len > max_line_length - 1)
			{
				return {{}, std::errc::invalid_argument};
			}
			if(len == 1 && m_unget_line[0] == '\0')
			{
				// empty line
				m_line = std::string_view();
			}
			else
			{
				// copy line
				std::memcpy(m_line_data.get(), m_unget_line.c_str(), len + 1);
				m_line = std::string_view(m_line_data.get(), len);
			}
			m_unget_line.clear();
		}
		else
		{
			if(err != std::errc{}) { return {{}, err}; }
			if(s->eof()) { return {{}, std::errc::io_error}; }
			if(!s->getline(m_line_data.get(), max_line_length))
			{
				if(s->eof() && s->gcount() == 0)
				{
					s->clear(std::ios::eofbit);
					// extracted no characters and reached end of file, return
					// no error
					return {{}, {}};
				}
				else
				{
					// error
					return {{}, std::errc::io_error};
				}
			}
			// get length, if eof is set, is last line and no delimiter was
			// extracted
			len = static_cast<size_t>(s->gcount()) - (s->eof() ? 0 : 1);
			if(len == 0) { m_line = std::string_view(); }
			else
			{
				if(std::iscntrl(m_line_data[len - 1]))
				{
					// mainly \r reading windows files in linux
					m_line_data[--len] = '\0';
				}
				m_line = std::string_view(m_line_data.get(), len);
			}
			m_line_num += 1;
		}
		if(skip_blanks && is_line_blank(m_line))
		{
			continue; // blank line, repeat
		}
		return {m_line, {}};
	}
}
void
serialize_base::unreadline(std::string_view line)
{
	if(!line.empty()) [[likely]] { m_unget_line = line; }
	else { m_unget_line = '\0'; }
}

bool
serialize_base::is_line_blank(std::string_view line)
{
	return std::all_of(line.begin(), line.end(), [](char c) {
		return std::isspace((unsigned char)c);
	});
}

} // namespace warthog::io
