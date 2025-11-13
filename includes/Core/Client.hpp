// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include "../../includes/Core/Server.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include "../../includes/Wrapper/SocketAddress.hpp"
#include <deque>
#include <fcntl.h>
#include <netinet/in.h>
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
		WAITING_FOR_EPOLLIN = 0,  // Ready to process incoming data
		WAITING_FOR_EPOLLOUT = 1, // Ready to send outgoing data
		DISCONNECTED = 2		  // Client has been disconnected
	};

private:
	// Objects
	FileDescriptor _clientFd;	  // File descriptor for the client
	SocketAddress _remoteAddress; // Remote address of the client

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
	void _routeRequest();

public:
	// Orchestrator methods
	Client();
	Client(FileDescriptor socketFd, SocketAddress remoteAddress);
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
