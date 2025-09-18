#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <algorithm>
#include <vector>
#include "FileDescriptor.hpp"
#include "RingBuffer.hpp"
#include "Logger.hpp"
#include "Constants.hpp"
#include "StringUtils.hpp"

// This class is responsible for the creation and management of files
// TODO: Implement file decode and encoding
class FileManager
{
private:
	std::string _filePath;
	FileDescriptor _fd;
	bool _isUsingTempFile;

	// File management methods
	std::string _generateTempFilePath();

public:
	FileManager();
	FileManager(FileManager const &src);
	~FileManager();

	FileManager &operator=(FileManager const &rhs);

	// Accessors
	std::string getFilePath() const;
	FileDescriptor getFd() const;
	bool getIsUsingTempFile() const;
	size_t getFileSize() const;

	// Mutators
	void setFilePath(const std::string &filePath);
	void setFd(const FileDescriptor &fd);
	void setIsUsingTempFile(bool isUsingTempFile);

	// Methods
	void append(const std::string &data);
	void append(const RingBuffer &buffer);
	void reset();
	void clear();
};

#endif /* ***************************************************** FILEMANAGER_H */
