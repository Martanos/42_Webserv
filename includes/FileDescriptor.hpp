#ifndef FILEDESCRIPTOR_HPP
#define FILEDESCRIPTOR_HPP

#include <iostream>
#include <string>
#include <unistd.h>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "Logger.hpp"

// Class wrapper for file descriptors
class FileDescriptor
{
private:
	int _fd;
	bool _isOpen;

public:
	explicit FileDescriptor(int fd = -1);
	FileDescriptor(FileDescriptor const &src);
	~FileDescriptor();
	FileDescriptor &operator=(FileDescriptor const &rhs);

	// Methods
	void closeDescriptor();

	// Accessors
	int getFd() const;
	bool isOpen() const;
	void setFd(int fd);

	// Verification
	bool isValid() const;
	bool isSocket() const;
	bool isPipe() const;
	bool isFile() const;
	bool isDirectory() const;
	bool isRegularFile() const;
	bool isSymbolicLink() const;

	// Socket operations
	void setNonBlocking();
	void setBlocking();
	void setCloseOnExec();
	void unsetCloseOnExec();
	void setReuseAddr();
	void unsetReuseAddr();

	// overloads
	operator int() const;

	// Comparator overloads
	bool operator==(const FileDescriptor &rhs) const;
	bool operator!=(const FileDescriptor &rhs) const;
	bool operator<(const FileDescriptor &rhs) const;
	bool operator>(const FileDescriptor &rhs) const;
	bool operator<=(const FileDescriptor &rhs) const;
	bool operator>=(const FileDescriptor &rhs) const;

	// TODO: File operations
};

std::ostream &operator<<(std::ostream &o, FileDescriptor const &i);

#endif /* ************************************************** FILEDESCRIPTOR_H */
