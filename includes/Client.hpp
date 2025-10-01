// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "DefaultStatusMap.hpp"
#include "FileDescriptor.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "Logger.hpp"
#include "MimeTypes.hpp"
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
	enum ClientState
	{
		CLIENT_WAITING_FOR_REQUEST = 0,
		CLIENT_PROCESSING_REQUESTS = 1,
		CLIENT_PROCESSING_RESPONSES = 2,
		CLIENT_DISCONNECTED = 3
	};

private:
	// Objects
	FileDescriptor _clientFd;  // File descriptor for the client
	SocketAddress _localAddr;  // Local address of the client (Who am I locally)
	SocketAddress _remoteAddr; // Remote address of the client (Who sent me)

	// Request and response caches and buffers
	HttpRequest _request;					  // Cached request (May be partially processed)
	HttpResponse _response;					  // Cached response (May be partially processed)
	std::deque<HttpResponse> _responseBuffer; // Buffer to hold responses

	// Raw data buffers
	std::vector<char> _receiveBuffer; // Static buffer to draw from the kernel buffer
	std::vector<char> _holdingBuffer; // Dynamic buffer to hold incoming data

	const std::vector<Server> *_potentialServers; // Potential servers to use for the request

	// State
	ClientState _state;	  // Current state of the client
	time_t _lastActivity; // Time of last activity will be used by server manager to check for timed out clients
	bool _keepAlive;	  // Whether the connection should be kept alive

	// Post header methods
	void _identifyServer();
	void _identifyCGI();

	// Request processing methods
	void _handleBuffer();
	void _handleRequest();

	// Response processing methods
	void _handleResponseBuffer();

	// TODO: Utility methods

public:
	// Orchestrator methods
	Client();
	Client(FileDescriptor socketFd, SocketAddress clientAddr, SocketAddress remoteAddr);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	// Core operations
	void handleEvent(epoll_event event);

	// State management

	// Mutators
	ClientState getCurrentState() const;
	void setState(ClientState newState);
	void updateActivity();

	// Accessors
	int getSocketFd() const;
	const SocketAddress &getLocalAddr() const;
	const SocketAddress &getRemoteAddr() const;
	const std::vector<Server> &getPotentialServers() const;
	void setPotentialServers(
		const std::vector<Server> &potentialServers); // For server manager to set potential servers
	bool isTimedOut() const;
};

// TODO: Stream overload for diagnostic purposes

#endif /* ********************************************************** CLIENT_H                                          \
		*/
