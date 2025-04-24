/*
 * file_utils.cpp
 *
 *  Created on: Oct 1, 2018
 *      Author: koldar
 */

#include <filesystem>
#include <fstream>
#include <stdio.h>
#include <warthog/util/file_utils.h>
#include <warthog/util/log.h>

namespace warthog::util
{

namespace
{
const char*
getBaseName_aux(const char* filepath)
{
	if(filepath == nullptr) return nullptr;
	const char* name_start = filepath;
	for(const char* p = filepath; *p != '\0'; ++p)
	{
		if(*p == '/' || *p == '\\') name_start = p + 1;
	}
	return name_start;
}
}

size_t
getBytesOfFile(const std::string& name)
{
	return std::filesystem::file_size(name);
}

bool
isFileExists(const std::string& name)
{
	return std::filesystem::exists(name);
}

std::string
getBaseNameAsString(const std::string& filepath)
{
	return getBaseName_aux(filepath.c_str());
}

std::string
getBaseNameAsString(const char* filepath)
{
	return getBaseName_aux(filepath);
}

const char*
getBaseName(const std::string& filepath)
{
	return getBaseName_aux(filepath.c_str());
}

const char*
getBaseName(const char* filepath)
{
	return getBaseName_aux(filepath);
}

} // namespace warthog::util
