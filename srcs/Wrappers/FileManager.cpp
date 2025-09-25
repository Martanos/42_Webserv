#include "../../includes/FileManager.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

FileManager::FileManager()
{
	_filePath = _generateTempFilePath();
	_fd = FileDescriptor(open(_filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
	_isUsingTempFile = true;
}

FileManager::FileManager(const FileManager &src)
{
	(void)src; // TODO: Implement proper copy constructor
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

FileManager::~FileManager()
{
	// Delete file if its stil a temp file
	if (_isUsingTempFile)
	{
		unlink(_filePath.c_str());
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
		_isUsingTempFile = rhs._isUsingTempFile;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

// FIXED FileManager::_generateTempFilePath() - Add this to FileManager.cpp

std::string FileManager::_generateTempFilePath()
{
	// CRITICAL: Maximum attempts to prevent infinite recursion
	const size_t MAX_ATTEMPTS = 1000;
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
				return ss.str();
			}

			// Use gmtime instead of localtime to avoid timezone files
			struct tm *tm = std::gmtime(&now);
			if (!tm)
			{
				// gmtime failed, use PID + counter fallback
				std::stringstream ss;
				ss << HTTP::TEMP_FILE_TEMPLATE << "pid" << getpid() << "_" << (++attemptCounter);
				return ss.str();
			}

			char buf[64];
			// Simple sprintf to avoid strftime issues
			int ret = std::sprintf(buf, "%04d%02d%02d%02d%02d%02d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
								   tm->tm_hour, tm->tm_min, tm->tm_sec);

			if (ret <= 0)
			{
				// sprintf failed, use PID + counter fallback
				std::stringstream ss;
				ss << HTTP::TEMP_FILE_TEMPLATE << "pid" << getpid() << "_" << (++attemptCounter);
				return ss.str();
			}

			// Make filename unique by adding PID and attempt counter
			std::stringstream ss;
			ss << HTTP::TEMP_FILE_TEMPLATE << buf << "_" << getpid() << "_" << attempt;
			std::string candidatePath = ss.str();

			// CORRECTED LOGIC: If file DOESN'T exist (access != 0), use it!
			if (access(candidatePath.c_str(), F_OK) != 0)
			{
				return candidatePath; // File doesn't exist, perfect!
			}

			// File exists, try next iteration with different counter
			// (The loop will automatically increment 'attempt')
		}
		catch (...)
		{
			// Any exception, use emergency fallback
			std::stringstream ss;
			ss << HTTP::TEMP_FILE_TEMPLATE << "emergency_" << getpid() << "_" << (++attemptCounter);
			return ss.str();
		}
	}

	// CRITICAL: If all attempts failed, return emergency fallback
	// This prevents infinite recursion absolutely
	std::stringstream ss;
	ss << HTTP::TEMP_FILE_TEMPLATE << "fallback_" << getpid() << "_" << time(0);
	Logger::log(Logger::WARNING, "All temp file generation attempts failed, using fallback: " + ss.str());
	return ss.str();
}
void FileManager::append(const std::string &data)
{
	_fd.writeFile(data);
}

void FileManager::append(const RingBuffer &buffer)
{
	std::string data;
	data.resize(buffer.readable());
	buffer.peekBuffer(data, buffer.readable());
	_fd.writeFile(data);
}

void FileManager::reset()
{
	unlink(_filePath.c_str());
	_fd = FileDescriptor(open(_filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
}

void FileManager::clear()
{
	unlink(_filePath.c_str());
	_fd = FileDescriptor(open(_filePath.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644));
}

size_t FileManager::contains(const char *data, size_t len) const
{
	// Read file and check if data is present
	std::string fileData;
	const_cast<FileDescriptor &>(_fd).readFile(fileData);
	return fileData.find(data, len) != std::string::npos;
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

std::string FileManager::getFilePath() const
{
	return _filePath;
}

FileDescriptor &FileManager::getFd()
{
	return _fd;
}

bool FileManager::getIsUsingTempFile() const
{
	return _isUsingTempFile;
}

size_t FileManager::getFileSize() const
{
	return _fd.getFileSize();
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

void FileManager::setIsUsingTempFile(bool isUsingTempFile)
{
	_isUsingTempFile = isUsingTempFile;
}

/* ************************************************************************** */
