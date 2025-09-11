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
#include "MimeTypes.hpp"

// This class represents the client connection it is simply the
// orchestrator of client operations
// Http requests are handled by http request class
// Request routing class takes care of method operations
// Http response class takes care of response generation and sending
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
		CLIENT_WRITING_FILE = 5,
		CLIENT_CLOSING = 6,
		CLIENT_DISCONNECTED = 7
	};

private:
	FileDescriptor _clientFd;
	int _socketFd;
	SocketAddress _clientAddr;
	State _currentState;

	HttpRequest _request;
	HttpResponse _response;
	std::vector<Server> &_potentialServers;
	Server *_server;

	std::string _readBuffer;
	time_t _lastActivity;
	bool _keepAlive;

	void _processHTTPRequest();
	void _generateErrorResponse(int statusCode, const std::string &message = "");
	void _identifyServer();

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

	// Accessors
	int getSocketFd() const;
	const std::string &getClientIP() const;
	const Server *getServer() const;
	const std::vector<Server> &getPotentialServers() const;
	void setPotentialServers(const std::vector<Server> &potentialServers) const;
	void setServer(const Server *server);
};

#endif /* ********************************************************** CLIENT_H */
