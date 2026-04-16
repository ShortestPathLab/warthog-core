#ifndef WARTHOG_UTIL_STRING_H
#define WARTHOG_UTIL_STRING_H

// template.h
//
// Utility string functions.
// Includes handling of space and conversion of string to other values,
// mainly int/double.
//
// @author: Ryan Hechenberger
// @created: 2026-04-14
//

#include <string_view>
#include <charconv>
#include <algorithm>
#include <concepts>
#include <cctype>
#include <cinttypes>
#include <cassert>
#include <span>
#include <string>

namespace warthog::util
{

/// @brief remove white spaces (as std::isspace) from beginning of string_view
inline std::string_view ltrim(std::string_view str)
{
	str.remove_prefix(std::find_if(str.begin(), str.end(), [](char c) { return !isspace((unsigned char)c); }) - str.begin());
	return str;
}

/// @brief remove white spaces (as std::isspace) from end of string_view
inline std::string_view rtrim(std::string_view str)
{
	str.remove_suffix(std::find_if(str.rbegin(), str.rend(), [](char c) { return !isspace((unsigned char)c); }) - str.rbegin());
	return str;
}

/// @brief string_view is either empty or only contains spaces (as std::isspace)
inline bool is_blank(std::string_view str)
{
	return std::all_of(str.begin(), str.end(), [](char c) { return isspace((unsigned char)c); });
}

struct token_return
{
	std::string_view token; ///< token extracted
	size_t trim; ///< amount of space discarded from front of string
};

inline token_return get_token(std::string_view str)
{
	auto fstr = ltrim(str);
	if (fstr.empty()) {
		return {std::string_view(), str.size()};
	}
	auto endsp = std::find_if(fstr.begin(), fstr.end(), [](char c) { return isspace((unsigned char)c); }) - fstr.begin();
	token_return ret;
	ret.token = fstr.substr(0, endsp);
	ret.trim = str.size() - fstr.size();
	return ret;
}

inline token_return get_token_quoted(std::string_view str, char quote = '"', char escape = '\\')
{
	if (quote == escape) {
		return {std::string_view(), 0};
	}
	auto fstr = ltrim(str);
	if (fstr.empty()) {
		return {std::string_view(), str.size()};
	}
	token_return ret;
	ret.trim = str.size() - fstr.size();
	if (fstr[0] == quote) {
		// quoted
		size_t len = 0;
		while(true){
			len = fstr.find(quote, len+1);
			if (len == std::string_view::npos) {
				// no quote close
				return {std::string_view(), ret.trim};
			}
			auto epos = fstr.find_last_not_of(escape, len-1) - len;
			if ((epos & 1) != 0) {
				// not delimited, return
				ret.token = fstr.substr(0, len+1);
				break;
			}
			// delimited, continue
		}
	} else {
		// no delimiter, read as normal
		auto endsp = std::find_if(str.begin(), str.end(), [](char c) { return isspace((unsigned char)c); }) - str.begin();
		ret.token = fstr.substr(0, endsp);
	}
	return ret;
}

inline std::errc parse_token(std::string_view token, std::integral auto& out, int base = 10)
{
	const char*const f = token.data();
	const char*const ed = token.data() + token.size();
	auto res = std::from_chars(token.data(), ed, out, base);
	if (res.ec != std::errc{})
		return res.ec;
	// value set
	if (res.ptr != ed) {
		return std::errc::invalid_argument;
	}
	return std::errc{};
}
inline std::errc parse_token(std::string_view token, std::floating_point auto& out, std::chars_format fmt = std::chars_format::general)
{
	const char*const ed = token.data() + token.size();
	auto res = std::from_chars(token.data(), ed, out, fmt);
	if (res.ec != std::errc{})
		return res.ec;
	// value set
	if (res.ptr != ed) {
		return std::errc::invalid_argument;
	}
	return std::errc{};
}

inline std::pair<std::string_view, std::errc> parse_token_part(std::string_view token, std::integral auto& out, int base = 10)
{
	std::from_chars_result res = std::from_chars(token.data(), token.data() + token.size(), out, base);
	if (res.ec != std::errc{})
		return {{}, res.ec};
	// value set
	token.remove_prefix(res.ptr - token.data());
	return {token, {}};
}
inline std::pair<std::string_view, std::errc> parse_token_part(std::string_view token, std::floating_point auto& out, std::chars_format fmt = std::chars_format::general)
{
	std::from_chars_result res = std::from_chars(token.data(), token.data() + token.size(), out, fmt);
	if (res.ec != std::errc{})
		return {{}, res.ec};
	// value set
	token.remove_prefix(res.ptr - token.data());
	return {token, {}};
}

inline std::pair<char*,std::errc> parse_token_quoted(std::string_view token, char* buffer, size_t size, char quote = '"', char escape = '\\')
{
	if (quote == escape) {
		return {nullptr, std::errc::invalid_argument};
	}
	if (token.empty() || size == 0) {
		return {nullptr, std::errc::invalid_argument};
	}
	const char*const ed = token.end() + token.size();
	if (token[0] != '"') {
		// not quoted, take as whole token
		if (std::any_of(token.data(), ed, [](char c) { return std::isspace((unsigned char)c); })) {
			// space detected, error
			return {nullptr, std::errc::invalid_argument};
		}
		if (token.size()+1 > size) {
			// too large to hold token
			return {nullptr, std::errc::value_too_large};
		}
		token.copy(buffer, token.size());
		buffer[token.size()] = '\0';
		return {buffer + token.size() + 1, {}};
	}
	// quoted text, process
	const char* t_at = token.data() + 1;
	while (true) {
		assert(size > 0);
		if (t_at >= ed) {
			// reached end of token with no end-quote
			return {buffer, std::errc::invalid_argument};
		}
		if (*t_at == '\\') [[unlikely]] {
			if (++t_at >= ed) {
				// unexpected end of string
				return {buffer, std::errc::invalid_argument};
			}
		} else if (*t_at == '"') [[unlikely]] {
			// found end quote, append null-terminator
			*(buffer++) = '\0';
			// check is end of token
			return {buffer, t_at+1 == ed ? std::errc{} : std::errc::invalid_argument};
		}
		*(buffer++) = *(t_at++);
		if (--size == 0) {
			// out of space for null-terminator
			return {buffer, std::errc::value_too_large};
		}
	}
}
inline std::errc parse_token_quoted(std::string_view token, auto& str_out, char quote = '"', char escape = '\\')
{
	str_out.resize(token.size()+1); // plus 1 to hold \0 placed in buffer
	auto [ed, ec] = parse_token_quoted(token, str_out.data(), str_out.size(), quote, escape);
	if (ec != std::errc{}) {
		// failed to read, clear str_out and return error code
		str_out.resize(0);
		return ec;
	} else {
		// successor, adjust str_out length and return
		assert(ed - str_out.data() > 0);
		str_out.resize((ed - str_out.data()) - 1);
		return std::errc{};
	}
}


class string_parser
{
public:
	string_parser();
	string_parser(std::string_view str) : m_str(str)
	{ }
	// error codes
	void clear_error() noexcept
	{
		m_error = std::errc{};
	}
	std::errc error() const noexcept
	{
		 return m_error;
	}
	bool is_error() const noexcept
	{
		return m_error != std::errc{};
	}
	operator bool() const noexcept
	{
		return m_error == std::errc{};
	}
	
	// set string
	std::string_view str() const noexcept
	{
		return m_str;
	}
	void str(std::string_view s) noexcept
	{
		m_str = s;
		m_token = std::string_view{};
		clear_error();
	}

	std::string_view last_token() const noexcept
	{
		return m_token;
	}

	string_parser& next(std::integral auto& int_out, int base = 10)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_())
			return *this;
		std::errc ec = util::parse_token(m_token, int_out, base);
		if (ec != std::errc{})
			m_error = ec;
		return *this;
	}
	string_parser& next(std::floating_point auto& float_out, std::chars_format fmt = std::chars_format::general)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_())
			return *this;
		std::errc ec = util::parse_token(m_token, float_out, fmt);
		if (ec != std::errc{})
			m_error = ec;
		return *this;
	}
	string_parser& next(std::span<char>& buffer)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (buffer.empty()) {
			m_error = std::errc::invalid_argument;
			return *this;
		}
		if (!next_token_())
			return *this;
		if (m_token.size() > buffer.size()) {
			m_error = std::errc::value_too_large;
			return *this;
		}
		m_token.copy(buffer.data(), m_token.size());
		buffer = buffer.subspan(0, m_token.size());
		return *this;
	}
	string_parser& next(std::string_view& out_token)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_())
			return *this;
		out_token = m_token;
		return *this;
	}
	string_parser& next(std::string& out_token)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_())
			return *this;
		out_token = m_token;
		return *this;
	}
	string_parser& next(std::pmr::string& out_token)
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_())
			return *this;
		out_token = m_token;
		return *this;
	}

	string_parser& next_q(std::span<char>& buffer, char quote = '"', char escape = '\\')
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (buffer.empty()) {
			m_error = std::errc::invalid_argument;
			return *this;
		}
		if (!next_token_quoted_(quote, escape))
			return *this;
		auto [ed, ec] = util::parse_token_quoted(m_token, buffer.data(), buffer.size(), quote, escape);
		if (ec != std::errc{}) {
			m_error = ec;
			return *this;
		}
		if (ed == buffer.data()) {
			assert(false); // should never occur
			m_error = std::errc::state_not_recoverable;
			return *this;
		}
		buffer = buffer.subspan(0, (ed - buffer.data()) - 1);
		return *this;
	}
	string_parser& next_q(std::string& out_token, char quote = '"', char escape = '\\')
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_quoted_(quote, escape))
			return *this;
		std::errc ec = util::parse_token_quoted(m_token, out_token, quote, escape);
		if (ec != std::errc{}) {
			m_error = ec;
			return *this;
		}
		return *this;
	}
	string_parser& next_q(std::pmr::string& out_token, char quote = '"', char escape = '\\')
	{
		if (is_error()) // do nothing if in error state
			return *this;
		if (!next_token_quoted_(quote, escape))
			return *this;
		std::errc ec = util::parse_token_quoted(m_token, out_token, quote, escape);
		if (ec != std::errc{}) {
			m_error = ec;
			return *this;
		}
		return *this;
	}
	string_parser& ignore()
	{
		if (is_error()) // do nothing if in error state
			return *this;
		next_token_();
		return *this;
	}
	string_parser& ignore_q(char quote = '"', char escape = '\\')
	{
		if (is_error()) // do nothing if in error state
			return *this;
		next_token_quoted_(quote, escape);
		return *this;
	}

	bool eof()
	{
		if (is_error())
			return false;
		if (!util::is_blank(m_str))
		{
			m_error = std::errc::invalid_argument;
			return false;
		}
		return true;
	}

protected:
	bool next_token_()
	{
		auto ret = util::get_token(m_str);
		if (ret.token.empty()) {
			// failed to read a token, error
			m_error = std::errc::invalid_argument;
			return false;
		} else {
			m_token = ret.token;
			assert(ret.trim <= m_str.size());
			m_str.remove_prefix(ret.token.size() + ret.trim);
			return true;
		}
	}
	bool next_token_quoted_(char quote, char escape)
	{
		auto ret = util::get_token_quoted(m_str, quote, escape);
		if (ret.token.empty()) {
			// failed to read a token, error
			m_error = std::errc::invalid_argument;
			return false;
		} else {
			m_token = ret.token;
			assert(ret.trim <= m_str.size());
			m_str.remove_prefix(ret.token.size() + ret.trim);
			return true;
		}
	}

protected:
	std::string_view m_str;
	std::string_view m_token;
	std::errc m_error = std::errc{};
};


} // namespace warthog::util

#endif // WARTHOG_UTIL_STRING_H
