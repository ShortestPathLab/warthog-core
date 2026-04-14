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
#include <cctype>
#include <cinttypes>

namespace warthog::util
{

/// @brief remove white spaces (as std::isspace) from beginning of string_view
std::string_view ltrim(std::string_view str)
{
	str.remove_prefix(std::find_if(str.begin(), str.end(), [](char c) { return !isspace((unsigned char)c); }) - str.begin());
}

/// @brief remove white spaces (as std::isspace) from end of string_view
std::string_view rtrim(std::string_view str)
{
	str.remove_suffix(std::find_if(str.rbegin(), str.rend(), [](char c) { return !isspace((unsigned char)c); }) - str.rbegin());
}

/// @brief string_view is either empty or only contains spaces (as std::isspace)
bool is_blank(std::string_view str)
{
	return std::all_of(str.begin(), str.end(), [](char c) { return isspace((unsigned char)c); });
}

struct token_return
{
	std::string_view token; ///< token extracted
	size_t trim; ///< amount of space discarded from front of string
};

token_return get_token(std::string_view str)
{
	auto fstr = ltrim(str);
	if (fstr.empty()) {
		return {std::string_view(), str.size()};
	}
	auto endsp = std::find_if(str.begin(), str.end(), [](char c) { return isspace((unsigned char)c); }) - str.begin();
	token_return ret;
	ret.token = fstr.substr(0, endsp);
	ret.trim = str.size() - fstr.size();
	return ret;
}

token_return get_token_quoted(std::string_view str, char quote = '"', char escape = '\\')
{
	if (quote == escape) {
		return {std::string_view(), str.size()};
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


} // namespace warthog::util

#endif // WARTHOG_UTIL_STRING_H
