#include "FileManager.hpp"

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

std::string FileManager::_generateTempFilePath()
{
	std::time_t now = std::time(0);
	char buf[80];
	std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&now));
	if (access((std::string(HTTP::TEMP_FILE_TEMPLATE) + std::string(buf)).c_str(), F_OK) == 0)
	{
		return std::string(HTTP::TEMP_FILE_TEMPLATE) + std::string(buf);
	}
	return _generateTempFilePath();
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

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

std::string FileManager::getFilePath() const
{
	return _filePath;
}

FileDescriptor FileManager::getFd() const
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
