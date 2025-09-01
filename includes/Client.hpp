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
		CLIENT_SENDING_RESPONSE = 3
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

	// Getters
	int getSocketFd() const;
	const std::string &getClientIP() const;
	const Server *getServer() const;
	void setServer(const Server *server);

private:
	void _processHTTPRequest();
	void _generateErrorResponse(int statusCode, const std::string &message = "");
	void _generateFileResponse(const std::string &filePath);
	void _generateDirectoryListing(const std::string &dirPath);
	bool _isMethodAllowed(const std::string &method) const;
	std::string _resolveFilePath(const std::string &uri) const;
	void _handleCGIRequest();
	void _handleFileUpload();
};

#endif /* ********************************************************** CLIENT_H */
