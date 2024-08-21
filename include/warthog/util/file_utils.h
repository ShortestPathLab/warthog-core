/*
 * file_utils.h
 *
 *  Created on: Oct 1, 2018
 *      Author: koldar
 */

#ifndef WARTHOG_UTIL_FILE_UTILS_H
#define WARTHOG_UTIL_FILE_UTILS_H

#include <string>

namespace warthog::util
{

/**
 * @brief get the number of bytes the file occipies on the filesyste,
 *
 * @param name the name fo the file to aqnalyze
 * @return size_t number of bytes @c name occupies
 */
[[deprecated("Move to std::filesystem")]]
size_t
getBytesOfFile(const std::string& name);

/**
 * Check if a file exists
 *
 * @param[in] name the filename to check
 * @return
 *  @li true fi the file exists;
 *  @li false otherwise;
 */
[[deprecated("Move to std::filesystem")]]
bool
isFileExists(const std::string& name);

/**
 * Get the basename of a file given its absolute path
 *
 * @pre
 *  @li filepath is an absolute path (like the one returneed by __FILE__);
 *
 * @param[in] filepath the absolute path to handle
 * @return a pointer in @c filepath where the basename of the file starts
 */
[[deprecated("Move to std::filesystem")]]
const char*
getBaseName(const char* filepath);

/**
 * @brief Get the basename of a file given its path
 *
 * @pre
 *  @li filepath does not end with "/"
 *
 * @param filepath  the path to handle
 * @return const char* a pointer of the given path
 */
[[deprecated("Move to std::filesystem")]]
const char*
getBaseName(const std::string& filepath);

/**
 * @brief Get the basename of a file given its path
 *
 * @pre
 *  @li filepath does not end with "/"
 *
 * @param filepath  the path to handle
 * @return a copy of the basename path
 */
[[deprecated("Move to std::filesystem")]]
std::string
getBaseNameAsString(const std::string& filepath);

/**
 * @brief Get the basename of a file given its path
 *
 * @pre
 *  @li filepath does not end with "/"
 *
 * @param filepath  the path to handle
 * @return a copy of the basename path
 */
[[deprecated("Move to std::filesystem")]]
std::string
getBaseNameAsString(const char* filepath);

} // namespace warthog::util

#endif // WARTHOG_UTIL_FILE_UTILS_H
