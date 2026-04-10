#ifndef WARTHOG_IO_SERIALIZE_BASE_H
#define WARTHOG_IO_SERIALIZE_BASE_H

// io/serialize_base.h
//
// Read/write base class for serialize classes.
// Adds a uniform low-level interface for reading from a file, one-line at a time.
// Support for both file or just from a istream/ostream object.
// Tracking of line number for user-level error reporting and debugging.
//
// @author: Ryan Hechenberger
// @created: 2026-04-10
//

#include <cassert>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>

namespace warthog::io
{

class serialize_base
{
public:
	static constexpr size_t max_line_length = 16 << 10; // 16kB
	serialize_base();
	virtual ~serialize_base();

	void
	set_filename(std::filesystem::path&& filename)
	{
		m_filename = std::move(filename);
	}
	const std::filesystem::path&
	get_filename() const noexcept
	{
		return m_filename;
	}

	int32_t
	get_line_num() const noexcept
	{
		return m_line_num;
	}
	std::string_view
	get_last_line() const noexcept
	{
		return m_line;
	}

	/// @brief opens scenario file get_scenario_filename() for reading
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	virtual std::errc
	open_read(std::istream* scenario = nullptr);
	/// @brief opens scenario file get_scenario_filename() for writing
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return error on operation
	virtual std::errc
	open_write(std::ostream* scenario = nullptr);

	virtual void
	close();

	/// @return if scenario is open for reading
	bool
	can_read(std::istream* in = nullptr)
	{
		if(in == nullptr) in = m_stream_in;
		return in != nullptr && !in->bad();
	}
	/// @return if scenario is open for writing
	bool
	can_write(std::ostream* out = nullptr)
	{
		if(out == nullptr) out = m_stream_out;
		return out != nullptr && !out->bad();
	}

protected:
	std::pair<std::istream*, std::errc>
	get_istream(std::istream* in = nullptr) noexcept
	{
		if(in == nullptr) in = m_stream_in;
		if(in == nullptr || !in->good()) return {nullptr, std::errc::io_error};
		return {in, {}};
	}
	std::pair<std::ostream*, std::errc>
	get_ostream(std::ostream* out = nullptr) noexcept
	{
		if(out == nullptr) out = m_stream_out;
		if(out == nullptr || !out->good())
			return {nullptr, std::errc::io_error};
		return {out, {}};
	}
	std::istream&
	line_stream(std::string_view line);
	bool
	line_stream_eof();
	std::pair<std::string_view, std::errc>
	readline(std::istream* in);
	void
	unreadline(std::string_view line);

protected:
	std::filesystem::path m_filename;
	std::unique_ptr<std::ios_base> m_stream;
	std::istream* m_stream_in  = nullptr;
	std::ostream* m_stream_out = nullptr;
	int32_t m_line_num    = -1;
	std::unique_ptr<char[]> m_line_data;
	std::string_view m_line;
	std::string m_unget_line;

	// shared temp parameter
	std::istringstream m_iss;
	std::string m_token;
};

} // namespace warthog::io

#endif // WARTHOG_IO_SERIALIZE_BASE_H
