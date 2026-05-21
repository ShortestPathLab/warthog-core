#include <warthog/io/serialize_base.h>

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <string>

namespace warthog::io
{

serialize_base::serialize_base() { }
serialize_base::~serialize_base() = default;

std::expected<void, std::errc>
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
		return std::unexpected(std::errc::io_error);
	}
	return {};
}

std::expected<void, std::errc>
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
		return std::unexpected(std::errc::io_error);
	}
	return {};
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
	in = get_istream(in).value_or(nullptr);
	return in != nullptr && in->eof();
}

std::expected<std::string_view, std::errc>
serialize_base::readline(std::istream* in, bool skip_blanks)
{
	size_t len;
	if (auto r = get_istream(in); r)
		in = *r;
	else
		return std::unexpected(r.error());
	if(!m_line_data)
	{
		m_max_line_length
		    = m_max_line_length > 0 ? m_max_line_length : max_line_length;
		m_line_data = std::make_unique<char[]>(m_max_line_length);
	}
	while(true)
	{
		if(len = m_unget_line.size(); len != 0)
		{
			// return last unreadline
			if(len > m_max_line_length - 1)
			{
				return std::unexpected(std::errc::invalid_argument);
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
			if(in->eof()) return std::unexpected(std::errc::io_error);
			if(!in->getline(m_line_data.get(), m_max_line_length))
			{
				if(in->eof() && in->gcount() == 0)
				{
					in->clear(std::ios::eofbit);
					// extracted no characters and reached end of file, return
					// no error
					return std::string_view();
				}
				else
				{
					// error
					return std::unexpected(std::errc::io_error);
				}
			}
			// get length, if eof is set, is last line and no delimiter was
			// extracted
			len = static_cast<size_t>(in->gcount()) - (in->eof() ? 0 : 1);
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
		return m_line;
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
