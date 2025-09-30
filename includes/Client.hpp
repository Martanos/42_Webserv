// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "DefaultStatusMap.hpp"
#include "FileDescriptor.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "MimeTypes.hpp"
#include "RequestRouter.hpp"
#include "Server.hpp"
#include "ServerMap.hpp"
#include "SocketAddress.hpp"
#include "StringUtils.hpp"
#include <deque>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

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
		CLIENT_SENDING_RESPONSE = 2,
		CLIENT_DISCONNECTED = 3
	};

private:
	// Objects
	FileDescriptor _clientFd;  // File descriptor for the client
	SocketAddress _localAddr;  // Local address of the client (Who am I locally)
	SocketAddress _remoteAddr; // Remote address of the client (Who sent me)

	// Request parsing objects
	HttpRequest _request;					  // The request itself
	HttpResponse _response;					  // The response itself
	std::deque<HttpResponse> _responseBuffer; // Buffer to hold responses

	std::vector<char> _receiveBuffer; // Static buffer to draw from the kernel buffer
	std::vector<char> _holdingBuffer; // Dynamic buffer to hold incoming data

	const std::vector<Server> *_potentialServers; // Potential servers to use for the request

	// State
	State _state;		  // Current state of the client
	time_t _lastActivity; // Time of last activity will be used by server manager to check for timed out clients
	bool _keepAlive;	  // Whether the connection should be kept alive

	void _identifyServer();
	void _identifyCGI();

	// Core operations
	void _readRequest();
	void _sendResponse();
	void _flushResponseQueue();
	void _processHTTPRequest();

public:
	Client();
	Client(FileDescriptor socketFd, SocketAddress clientAddr, SocketAddress remoteAddr);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	// Core operations
	void handleEvent(epoll_event event);

	// State management
	State getCurrentState() const;
	void setState(State newState);
	bool isTimedOut() const;
	void updateActivity();

	// Accessors
	int getSocketFd() const;
	const SocketAddress &getLocalAddr() const;
	const SocketAddress &getRemoteAddr() const;
	const std::vector<Server> &getPotentialServers() const;
	void setPotentialServers(const std::vector<Server> &potentialServers);
	void setServer(const Server *server);
};

#endif /* ********************************************************** CLIENT_H                                          \
		*/
