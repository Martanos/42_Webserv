#ifndef FILEUTILS_HPP
#define FILEUTILS_HPP

#include "../../includes/Utils/StrUtils.hpp"
#include <cerrno>
#include <fcntl.h>
#include <limits.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace FileUtils
{
/* File operation checks*/

static inline bool isFileReadable(const std::string &filePath)
{
	errno = 0;
	return access(filePath.c_str(), R_OK) == 0;
}

static inline bool isFileWritable(const std::string &filePath)
{
	errno = 0;
	return access(filePath.c_str(), W_OK) == 0;
}

static inline bool isFileExecutable(const std::string &filePath)
{
	errno = 0;
	return access(filePath.c_str(), X_OK) == 0;
}
static inline bool fileExists(const std::string &filePath)
{
	errno = 0;
	return access(filePath.c_str(), F_OK) == 0;
}

/* File path modifiers */

static inline std::string normalizePath(const std::string &filePath)
{
	errno = 0;
	char resolvedPath[PATH_MAX];
	if (realpath(filePath.c_str(), resolvedPath) == NULL)
	{
		return "";
	}
	return std::string(resolvedPath);
}

/* FilePath Extraction methods */

static inline std::string getFileExtension(const std::string &filePath)
{
	size_t dotPos = filePath.find_last_of('.');
	if (dotPos == std::string::npos || dotPos == filePath.length() - 1)
	{
		return "";
	}
	return filePath.substr(dotPos);
}

static inline std::string getFileName(const std::string &filePath)
{
	size_t slashPos = filePath.find_last_of('/');
	if (slashPos == std::string::npos)
	{
		return filePath;
	}
	return filePath.substr(slashPos + 1);
}

// Returns the resolved canonical path of the symlink target
static inline std::string traverseSymlink(const std::string &linkPath)
{
	errno = 0;
	char resolvedPath[PATH_MAX];
	if (realpath(linkPath.c_str(), resolvedPath) == NULL)
	{
		return "";
	}
	return std::string(resolvedPath);
}

/* File path checks */

// Assume paths are normalized and absolute
static inline bool inRoot(const std::string &rootPath, const std::string &targetPath)
{
	if (rootPath.empty() || targetPath.empty())
	{
		return false;
	}
	if (targetPath.compare(0, rootPath.length(), rootPath) != 0)
	{
		return false;
	}
	// Ensure that target is either the root itself or a subpath
	if (targetPath.length() > rootPath.length() && targetPath[rootPath.length()] != '/')
	{
		return false;
	}
	return true;
}

/* File type checks */

enum FileType
{
	NOT_FOUND = 0,
	REGULAR_FILE = S_IFREG,
	DIRECTORY = S_IFDIR,
	SYMBOLIC_LINK = S_IFLNK,
	CHARACTER_DEVICE = S_IFCHR,
	BLOCK_DEVICE = S_IFBLK,
	FIFO_PIPE = S_IFIFO,
	SOCKET = S_IFSOCK,
	UNKNOWN = -1
};

static inline FileType getFileType(const std::string &path)
{
	struct stat pathStat;
	if (lstat(path.c_str(), &pathStat) != 0)
		return NOT_FOUND;
	switch (pathStat.st_mode & S_IFMT)
	{
	case S_IFREG:
		return REGULAR_FILE;
	case S_IFDIR:
		return DIRECTORY;
	case S_IFLNK:
		return SYMBOLIC_LINK;
	case S_IFCHR:
		return CHARACTER_DEVICE;
	case S_IFBLK:
		return BLOCK_DEVICE;
	case S_IFIFO:
		return FIFO_PIPE;
	case S_IFSOCK:
		return SOCKET;
	default:
		return UNKNOWN;
	}
}

/* File size retrieval */

static inline off_t getFileSize(const std::string &filePath)
{
	struct stat fileStat;
	if (stat(filePath.c_str(), &fileStat) != 0)
	{
		return -1;
	}
	return fileStat.st_size;
}

/* Generate a unique file name */
static inline std::string generateFileName(const std::string &directory, const std::string &prefix,
										   const std::string &extension)
{
	std::string fileName;
	do
	{
		if (!prefix.empty())
			fileName = directory + "/" + prefix + "_" + StrUtils::toString(time(NULL)) + "_" +
					   StrUtils::toString(rand()) + extension;
		else
			fileName = directory + "/" + StrUtils::toString(time(NULL)) + "_" + StrUtils::toString(rand()) + extension;
	} while (fileExists(fileName));
	return fileName;
}

/* Write to file */
static inline bool writeToFile(const std::string &filePath, const std::string &data, mode_t mode = 0644)
{
	int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, mode);
	if (fd == -1)
		return false;
	const char *cursor = data.data();
	size_t remaining = data.size();
	while (remaining > 0)
	{
		ssize_t written = write(fd, cursor, remaining);
		if (written == -1)
		{
			if (errno == EINTR)
				continue;
			close(fd);
			return false;
		}
		cursor += written;
		remaining -= written;
	}
	if (fsync(fd) == -1)
	{
		int syncError = errno;
		close(fd);
		errno = syncError;
		return false;
	}
	close(fd);
	return true;
}
} // namespace FileUtils

#endif /* FILEUTILS_HPP */
