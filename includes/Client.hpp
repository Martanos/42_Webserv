// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "Server.hpp"
#include "Logger.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "FileDescriptor.hpp"
#include "SocketAddress.hpp"
#include "ServerMap.hpp"
#include "StringUtils.hpp"
#include "RequestRouter.hpp"
#include "DefaultStatusMap.hpp"
// This class is used to handle the client connection and process the HTTP request
class Client
{
public:
	enum State
	{
		CLIENT_WAITING_FOR_REQUEST = 0,
		CLIENT_READING_REQUEST = 1,
		CLIENT_PROCESSING_REQUEST = 2,
		CLIENT_SENDING_RESPONSE = 3,
		CLIENT_READING_FILE = 4,
		CLIENT_WRITING_FILE = 5
	};

	enum FileOperationState
	{
		FILE_OP_NONE = 0,
		FILE_OP_OPENING = 1,
		FILE_OP_READING = 2,
		FILE_OP_WRITING = 3,
		FILE_OP_COMPLETE = 4,
		FILE_OP_ERROR = 5
	};

private:
	FileDescriptor _socketFd;
	SocketAddress _clientAddr;
	State _currentState;

	HttpRequest _request;
	HttpResponse _response;
	Server *_server;

	std::string _readBuffer;
	time_t _lastActivity;
	bool _keepAlive;

	FileOperationState _fileOpState;
	FileDescriptor _fileOpFd;
	size_t _fileOffset;
	size_t _responseBytesSent;
	size_t _fileSize;
	std::string _filePath;

	static const size_t FILE_CHUNK_SIZE = 8192; // 8KB

public:
	Client();
	Client(FileDescriptor socketFd, SocketAddress clientAddr);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	// Core operations
	void handleEvent(epoll_event event);
	void readRequest();
	void sendResponse();

	// State management
	State getCurrentState() const;
	void setState(State newState);
	bool isTimedOut() const;
	void updateActivity();

	// File operations
	void startFileOperation(const std::string &filePath);
	void readFileChunk();
	void finishFileOperation();

	// Getters
	int getSocketFd() const;
	const std::string &getClientIP() const;
	const Server *getServer() const;
	void setServer(const Server *server);

private:
	void _processHTTPRequest();
	void _generateErrorResponse(int statusCode, const std::string &message = "");
};

#endif /* ********************************************************** CLIENT_H */
