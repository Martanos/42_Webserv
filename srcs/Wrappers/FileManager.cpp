#include "../../includes/Wrappers/FileManager.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
static std::string getParentDirectory(const std::string &path)
{
	size_t slash = path.find_last_of('/');
	if (slash == std::string::npos)
		return std::string();
	if (slash == 0)
		return "/";
	return path.substr(0, slash);
}

static bool directoryExists(const std::string &path)
{
	if (path.empty())
		return true;
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

static bool ensureParentDirectory(const std::string &destination)
{
	std::string parent = getParentDirectory(destination);
	if (parent.empty())
		return true;
	return directoryExists(parent);
}

static void closeSilently(int fd)
{
	if (fd >= 0)
		close(fd);
}

static bool copyFileAcrossMounts(const std::string &source, const std::string &destination, bool overwrite)
{
	int srcFd = open(source.c_str(), O_RDONLY);
	if (srcFd == -1)
		return false;
	int destFlags = O_WRONLY | O_CREAT;
	destFlags |= overwrite ? O_TRUNC : O_EXCL;
	int destFd = open(destination.c_str(), destFlags, 0644);
	if (destFd == -1)
	{
		closeSilently(srcFd);
		return false;
	}
	char buffer[65536];
	while (true)
	{
		ssize_t bytesRead = read(srcFd, buffer, sizeof(buffer));
		if (bytesRead == 0)
			break;
		if (bytesRead == -1)
		{
			if (errno == EINTR)
				continue;
			int readError = errno;
			closeSilently(srcFd);
			closeSilently(destFd);
			unlink(destination.c_str());
			errno = readError;
			return false;
		}
		ssize_t written = 0;
		while (written < bytesRead)
		{
			ssize_t chunk = write(destFd, buffer + written, bytesRead - written);
			if (chunk == -1)
			{
				if (errno == EINTR)
					continue;
				int writeError = errno;
				closeSilently(srcFd);
				closeSilently(destFd);
				unlink(destination.c_str());
				errno = writeError;
				return false;
			}
			written += chunk;
		}
	}
	if (fsync(destFd) == -1)
	{
		int syncError = errno;
		closeSilently(srcFd);
		closeSilently(destFd);
		unlink(destination.c_str());
		errno = syncError;
		return false;
	}
	struct stat srcStat;
	if (fstat(srcFd, &srcStat) == 0)
		fchmod(destFd, srcStat.st_mode & 0777);
	closeSilently(srcFd);
	closeSilently(destFd);
	if (unlink(source.c_str()) == -1)
		return false;
	return true;
}
} // namespace

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

FileManager::FileManager()
{
	_isATempFile = true;
	_instantiated = false;
}

FileManager::FileManager(const FileManager &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

FileManager::~FileManager()
{
	// Delete file if its stil a temp file
	if (_isATempFile && _instantiated)
	{
		destroy();
	}
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

FileManager &FileManager::operator=(FileManager const &rhs)
{
	if (this != &rhs)
	{
		_filePath = rhs._filePath;
		_fd = rhs._fd;
		_isATempFile = rhs._isATempFile;
		_instantiated = rhs._instantiated;
	}
	return (*this);
}

/*
** --------------------------------- METHODS ----------------------------------
*/

std::string FileManager::_generateTempFilePath()
{
	const size_t MAX_ATTEMPTS = 1000;
	struct tm *tm;
	char buf[64];
	int ret;

	// CRITICAL: Maximum attempts to prevent infinite recursion
	static size_t attemptCounter = 0; // C++98 static counter
	for (size_t attempt = 0; attempt < MAX_ATTEMPTS; ++attempt)
	{
		try
		{
			// Generate base timestamp (use gmtime to avoid timezone issues)
			std::time_t now = std::time(0);
			if (now == static_cast<std::time_t>(-1))
			{
				// time() failed, use PID + counter fallback
				std::stringstream ss;
				ss << HTTP::TEMP_FILE_TEMPLATE << "pid" << getpid() << "_" << (++attemptCounter);
				return (ss.str());
			}
			// Use gmtime instead of localtime to avoid timezone files
			tm = std::gmtime(&now);
			if (!tm)
			{
				// gmtime failed, use PID + counter fallback
				std::stringstream ss;
				ss << HTTP::TEMP_FILE_TEMPLATE << "pid" << getpid() << "_" << (++attemptCounter);
				return (ss.str());
			}
			// Simple sprintf to avoid strftime issues
			ret = std::sprintf(buf, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
							   tm->tm_hour, tm->tm_min, tm->tm_sec);
			if (ret <= 0)
			{
				// sprintf failed, use PID + counter fallback
				std::stringstream ss;
				ss << HTTP::TEMP_FILE_TEMPLATE << "pid" << getpid() << "_" << (++attemptCounter);
				return (ss.str());
			}
			// Make filename unique by adding PID and attempt counter
			std::stringstream ss;
			ss << HTTP::TEMP_FILE_TEMPLATE << buf << "_" << getpid() << "_" << attempt;
			std::string candidatePath = ss.str();
			// CORRECTED LOGIC: If file DOESN'T exist (access != 0), use it!
			if (access(candidatePath.c_str(), F_OK) != 0)
			{
				return (candidatePath); // File doesn't exist, perfect!
			}
			// File exists, try next iteration with different counter
			// (The loop will automatically increment 'attempt')
		}
		catch (...)
		{
			// Any exception, use emergency fallback
			std::stringstream ss;
			ss << HTTP::TEMP_FILE_TEMPLATE << "emergency_" << getpid() << "_" << (++attemptCounter);
			return (ss.str());
		}
	}
	// CRITICAL: If all attempts failed, return emergency fallback
	// This prevents infinite recursion absolutely
	std::stringstream ss;
	ss << HTTP::TEMP_FILE_TEMPLATE << "fallback_" << getpid() << "_" << time(0);
	Logger::error("FileManager: Failed to generate unique temp file path after "
				  "maximum attempts. Using fallback path.",
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	return (ss.str());
}

void FileManager::instantiate()
{
	_filePath = _generateTempFilePath();
	_fd = FileDescriptor::createFromOpen(_filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
	_instantiated = true;
}

void FileManager::destroy()
{
	unlink(_filePath.c_str());
	_instantiated = false;
}

void FileManager::append(const std::string &data)
{
	if (!_instantiated)
		instantiate();
	_fd.writeFile(data);
}

void FileManager::append(const std::vector<char> &buffer, std::vector<char>::iterator start,
						 std::vector<char>::iterator end)
{
	if (!_instantiated)
		instantiate();
	_fd.writeFile(buffer, start, end);
}

void FileManager::reset()
{
	unlink(_filePath.c_str());
	_instantiated = false;
}

void FileManager::clear()
{
	unlink(_filePath.c_str());
	_instantiated = false;
}

size_t FileManager::contains(const char *data, size_t len) const
{
	// Read file and check if data is present
	std::string fileData;
	const_cast<FileDescriptor &>(_fd).readFile(fileData);
	return (fileData.find(data, len) != std::string::npos);
}

bool FileManager::moveTo(const std::string &destination, bool overwrite)
{
	if (!_instantiated)
	{
		Logger::error("FileManager: Cannot move file because it has not been instantiated", __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		return false;
	}
	if (destination.empty())
	{
		Logger::error("FileManager: Destination path is empty", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		errno = EINVAL;
		return false;
	}
	if (!ensureParentDirectory(destination))
	{
		Logger::error("FileManager: Destination directory does not exist: " + destination, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		errno = ENOENT;
		return false;
	}
	if (!overwrite && access(destination.c_str(), F_OK) == 0)
	{
		Logger::error("FileManager: Destination already exists and overwrite is "
					  "disabled: " +
						  destination,
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		errno = EEXIST;
		return false;
	}

	if (_fd.isOpen())
	{
		if (fsync(_fd.getFd()) == -1)
		{
			Logger::error("FileManager: Failed to fsync temp file before move: " + std::string(strerror(errno)),
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return false;
		}
		_fd.closeDescriptor();
	}

	if (rename(_filePath.c_str(), destination.c_str()) != 0)
	{
		int renameError = errno;
		if (renameError != EXDEV)
		{
			Logger::error("FileManager: Failed to rename temp file: " + std::string(strerror(renameError)), __FILE__,
						  __LINE__, __PRETTY_FUNCTION__);
			errno = renameError;
			return false;
		}
		if (!copyFileAcrossMounts(_filePath, destination, overwrite))
		{
			Logger::error("FileManager: Failed to copy temp file across mounts: " + std::string(strerror(errno)),
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			return false;
		}
	}

	_filePath = destination;
	_isATempFile = false;
	_instantiated = true;

	FileDescriptor reopened = FileDescriptor::createFromOpen(_filePath.c_str(), O_RDWR);
	if (!reopened.isOpen())
	{
		Logger::error("FileManager: Failed to reopen file after move: " + std::string(strerror(errno)), __FILE__,
					  __LINE__, __PRETTY_FUNCTION__);
		return false;
	}
	_fd = reopened;
	return true;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

std::string FileManager::getFilePath() const
{
	return (_filePath);
}

FileDescriptor &FileManager::getFd()
{
	return (_fd);
}

const FileDescriptor &FileManager::getFd() const
{
	return (_fd);
}

bool FileManager::getIsATempFile() const
{
	return (_isATempFile);
}

size_t FileManager::getFileSize() const
{
	if (!_instantiated)
		return (0);

	struct stat fileStat;
	if (fstat(_fd.getFd(), &fileStat) != 0)
		return (0);

	return (fileStat.st_size);
}

/*
** --------------------------------- MUTATOR ---------------------------------
*/

void FileManager::setFilePath(const std::string &filePath)
{
	_filePath = filePath;
}

void FileManager::setFd(const FileDescriptor &fd)
{
	_fd = fd;
}

void FileManager::setIsATempFile(bool isATempFile)
{
	_isATempFile = isATempFile;
}

/* ************************************************************************** */
