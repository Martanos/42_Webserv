#ifndef FILEDESCRIPTOR_HPP
#define FILEDESCRIPTOR_HPP

#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Class wrapper for file descriptors
class FileDescriptor
{
private:
	struct Control
	{
		int fd;
		double count;
		Control(int fd) : fd(fd), count(1){};
	};
	Control *_ctrl;

public:
	FileDescriptor();
	FileDescriptor(int fd);
	FileDescriptor(FileDescriptor const &src);
	~FileDescriptor();
	FileDescriptor &operator=(FileDescriptor const &rhs);

	// Methods
	void closeDescriptor();

	// Accessors
	int getFd() const;
	bool isOpen() const;
	void setFd(int fd);
	size_t getFileSize() const;

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
	bool operator==(int rhs) const;
	bool operator!=(int rhs) const;
	bool operator<(int rhs) const;
	bool operator>(int rhs) const;
	bool operator<=(int rhs) const;
	bool operator>=(int rhs) const;
	bool operator==(const FileDescriptor &rhs) const;
	bool operator!=(const FileDescriptor &rhs) const;
	bool operator<(const FileDescriptor &rhs) const;
	bool operator>(const FileDescriptor &rhs) const;
	bool operator<=(const FileDescriptor &rhs) const;
	bool operator>=(const FileDescriptor &rhs) const;

	// File operations
	ssize_t readFile(std::string &buffer);
	ssize_t writeFile(const std::string &buffer);

	// Socket operations
	ssize_t receiveData(void *buffer, size_t size);
	ssize_t sendData(const std::string &buffer);

	// Pipe operations
	ssize_t readPipe(std::string &buffer, size_t maxSize = 0);
	ssize_t writePipe(const std::string &buffer);
	bool waitForPipeReady(bool forReading, int timeoutMs = 1000) const;

	// Pipe creation utilities
	static bool createPipe(FileDescriptor &readEnd, FileDescriptor &writeEnd);
	static bool createPipeNonBlocking(FileDescriptor &readEnd, FileDescriptor &writeEnd);
};

std::ostream &operator<<(std::ostream &o, FileDescriptor const &i);

#endif /* ************************************************** FILEDESCRIPTOR_H                                          \
		*/
