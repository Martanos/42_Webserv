<<<<<<< HEAD:includes/FileManager.hpp
#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "Constants.hpp"
#include "FileDescriptor.hpp"
#include "Logger.hpp"
#include "RingBuffer.hpp"
#include "StringUtils.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// This class is responsible for the creation and management of files
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
	FileDescriptor &getFd();
	bool getIsUsingTempFile() const;
	size_t getFileSize() const;
	size_t contains(const char *data, size_t len) const;

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

#endif /* ***************************************************** FILEMANAGER_H                                          \
		*/
=======
#ifndef FILEMANAGER_HPP
#define FILEMANAGER_HPP

#include "../../includes/Global/Logger.hpp"
#include "FileDescriptor.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// TODO: make this a static helper class
class FileManager
{
private:
	std::string _filePath;
	FileDescriptor _fd;
	bool _isATempFile;	// Is this a temp file?
	bool _instantiated; // Is this an instantiated file?

	// File management methods
	std::string _generateTempFilePath();
	void instantiate();
	void destroy();

public:
	FileManager();
	FileManager(FileManager const &src);
	~FileManager();

	FileManager &operator=(FileManager const &rhs);

	// Accessors
	std::string getFilePath() const;
	FileDescriptor &getFd();
	const FileDescriptor &getFd() const;
	bool getIsATempFile() const;
	size_t getFileSize() const;
	size_t contains(const char *data, size_t len) const;

	// Mutators
	void setFilePath(const std::string &filePath);
	void setFd(const FileDescriptor &fd);
	void setIsATempFile(bool isATempFile);

	// Methods
	void append(const std::string &data);
	void append(const std::vector<char> &buffer, std::vector<char>::iterator start, std::vector<char>::iterator end);
	void reset();
	void clear();
};

#endif /* ***************************************************** FILEMANAGER_H                                          \
		*/
>>>>>>> ConfigParserRefactor:includes/Wrapper/FileManager.hpp
