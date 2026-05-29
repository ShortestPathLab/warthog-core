#ifndef WARTHOG_IO_SERIALIZE_BASE_H
#define WARTHOG_IO_SERIALIZE_BASE_H

/// @file serialize_base.h
///
/// Read/write base class for serialize classes.
/// Adds a uniform low-level interface for reading from a file, one-line at a
/// time. Support for both file or just from a istream/ostream object. Tracking
/// of line number for user-level error reporting and debugging.
///
/// Ideal for use with serialize_base::parser (util::string_parser),
/// which supports fast reading of tokens through string_view and error
/// checking.
///
/// @author: Ryan Hechenberger
/// @created: 2026-04-10

#include "fwd.h"

#include <warthog/util/string.h>

#include <cassert>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>

namespace warthog::io
{

/// @brief serialize base class to support line-by-line reading with strict
///        error checking, line-number tracking and other features.
///
/// The base class for serialize, low-level with strict error checking.
/// The ideal use case is to ensure data is read correctly, and error on
/// ill-formed without raising exceptions.
///
/// To support low-level, each read/write takes istream|ostream pointer to
/// override the class default-defined, use get_istream|get_ostream to handle
/// the selection manually. Most functions return std::errc of non-value init
/// to state that procedure has failed.
///
/// Use readline() to raed next line (or next blank line with true), limits
/// length to max_line_length (1k def) and strips \r?\n from end. Use
/// unreadline() to put a line back to be read by next readline (can be
/// changed).
class serialize_base
{
public:
	using parser = util::string_parser;

	static constexpr size_t max_line_length = 1
	    << 10; ///< 1kB default maximum line length
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
	/// @return errc on error
	virtual std::expected<void, std::errc>
	open_read(std::istream* scenario = nullptr);
	/// @brief opens scenario file get_scenario_filename() for writing
	/// @param scenario use a user provided instead of get_scenario_filename()
	/// @return errc on error
	virtual std::expected<void, std::errc>
	open_write(std::ostream* scenario = nullptr);

	virtual void
	close();

	/// @return if scenario is open for reading
	bool
	can_read(std::istream* in = nullptr)
	{
		if(in == nullptr) in = m_stream_in;
		return in != nullptr;
	}
	/// @return if scenario is open for writing
	bool
	can_write(std::ostream* out = nullptr)
	{
		if(out == nullptr) out = m_stream_out;
		return out != nullptr;
	}

	/// @return the maximum line length (null terminator included), 0 means
	/// unallocated and will default to max_line_length on readline() call
	uint32_t
	get_max_line_length() noexcept
	{
		return m_max_line_length;
	}

protected:
	/// @return the internal istream or provided, or error
	std::expected<std::istream*, std::errc>
	get_istream(std::istream* in = nullptr) noexcept
	{
		if(in == nullptr) in = m_stream_in;
		if(in == nullptr || !in) std::unexpected(std::errc::io_error);
		return in;
	}
	/// @return the internal istream or provided, or error
	std::expected<std::ostream*, std::errc>
	get_ostream(std::ostream* out = nullptr) noexcept
	{
		if(out == nullptr) out = m_stream_out;
		if(out == nullptr || !out->good())
			return std::unexpected(std::errc::io_error);
		return out;
	}

	/// @return true if istream is at eof, false if not or in error
	bool
	istream_eof(std::istream* in = nullptr);

	/// @brief reads lines and return 1 line, checking for errors
	/// @param in optional stream to use, otherwise uses internal-set stream
	/// @param skip_blanks if true, the first line where is_line_blank(line) is
	/// false is returned
	/// @return the first valid line (can be blank), or error, the final empty
	/// line is returned empty and eof is set
	///
	/// Reads lines into a buffer (max size get_max_line_length()), removing
	/// the line ends \r\n. If unreadline was called before, returns that line,
	/// otherwise reads from istream. Change the max buffer size with
	/// set_max_line_length, although it clears the buffer and current line.
	///
	/// If line does not fit, return an error.
	/// If an empty line is discovered, it will return the same result as the
	/// eof condition, use istream_eof to distinguish. The file can end of a
	/// a blank line seamlessly.
	///
	/// get_line_num() will return the 1-index line number, reading from an
	/// unreadline does not change the line number.
	std::expected<std::string_view, std::errc>
	readline(std::istream* in, bool skip_blanks = false);

	/// @brief unreads a line, keeps only a single line
	/// @param line the line to return and read later, does not have to match a
	/// line read from readline.
	void
	unreadline(std::string_view line);

	/// @brief check if while line is either blank or empty, as per
	/// std::isspace
	bool
	is_line_blank(std::string_view line);

	/// @brief sets the max line length, clears current line buffer and line
	void
	set_max_line_length(uint32_t len)
	{
		m_max_line_length = len;
		m_line_data       = nullptr;
		m_line            = std::string_view();
	}

protected:
	std::filesystem::path m_filename;
	std::unique_ptr<std::ios_base> m_stream;
	std::istream* m_stream_in  = nullptr;
	std::ostream* m_stream_out = nullptr;
	int32_t m_line_num         = 0;
	uint32_t m_max_line_length = 0;
	std::unique_ptr<char[]> m_line_data;
	std::string_view m_line;
	std::string m_unget_line;
};

/// @brief a serialize_base used solely for reading line-by-line
class line_serialize final : public serialize_base
{
public:
	using serialize_base::serialize_base;

	using serialize_base::is_line_blank;
	using serialize_base::istream_eof;
	using serialize_base::readline;
	using serialize_base::set_max_line_length;
	using serialize_base::unreadline;
};

} // namespace warthog::io

#endif // WARTHOG_IO_SERIALIZE_BASE_H
