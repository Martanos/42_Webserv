#ifndef FILEUTILS_HPP
#define FILEUTILS_HPP

#include <fcntl.h>
#include <string>
#include <unistd.h>

namespace FileUtils
{
static inline bool isFileReadable(const std::string &filePath)
{
	return access(filePath.c_str(), R_OK) == 0;
}

static inline bool isFileWritable(const std::string &filePath)
{
	return access(filePath.c_str(), W_OK) == 0;
}

static inline bool isFileExecutable(const std::string &filePath)
{
	return access(filePath.c_str(), X_OK) == 0;
}

static inline std::string normalizePath(const std::string &filePath)
{
	char resolvedPath[4096];
	if (realpath(filePath.c_str(), resolvedPath) == NULL)
	{
		return "";
	}
	return std::string(resolvedPath);
}
} // namespace FileUtils

#endif /* FILEUTILS_HPP */