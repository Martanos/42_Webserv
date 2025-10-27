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
#include <vector>

// Class wrapper for file descriptors
class FileDescriptor
{
private:
	struct Control
	{
		int fd;
		int count;
		Control(int fd) : fd(fd), count(1){};
	};
	Control *_ctrl;

	FileDescriptor(int fd); // Private constructor for factory methods

public:
	FileDescriptor();
	FileDescriptor(FileDescriptor const &src);
	~FileDescriptor();
	FileDescriptor &operator=(FileDescriptor const &rhs);

	// Methods
	void closeDescriptor();

	// Accessors
	int getFd() const;
	bool isOpen() const;
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
	std::string readFile() const;
	ssize_t writeFile(const std::string &buffer);
	ssize_t writeFile(const std::vector<char> &buffer, std::vector<char>::iterator start,
					  std::vector<char>::iterator end);
	// Socket operations
	ssize_t receiveData(void *buffer, size_t size);
	ssize_t sendData(const std::string &buffer);

	// Pipe operations
	ssize_t readPipe(std::string &buffer, size_t maxSize = 0);
	ssize_t writePipe(const std::string &buffer);
	bool waitForPipeReady(bool forReading, int timeoutMs = 1000) const;

	// Pipe creating utilities
	static bool createPipe(FileDescriptor &readEnd, FileDescriptor &writeEnd);

	// Static factory methods for all fd-creating functions
	static FileDescriptor createSocket(int domain, int type, int protocol);
	static FileDescriptor createFromAccept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
	static FileDescriptor createFromOpen(const char *pathname, int flags);
	static FileDescriptor createFromOpen(const char *pathname, int flags, mode_t mode);
	static FileDescriptor createFromDup(int oldfd);
	static FileDescriptor createFromDup2(int oldfd, int newfd);
	static FileDescriptor createFromOpendir(const char *name); // Returns fd from dirfd()
	static FileDescriptor createEpoll(int flags);
};

std::ostream &operator<<(std::ostream &o, FileDescriptor const &i);

#endif /* ************************************************** FILEDESCRIPTOR_H                                          \
		*/
