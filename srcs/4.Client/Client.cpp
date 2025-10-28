#include "../../includes/Core/Client.hpp"
#include "../../includes/Core/GetHandler.hpp"
#include "../../includes/Core/MethodHandlerFactory.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <cstdio>
#include <sstream>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Client::Client()
{
	_clientFd = FileDescriptor();
	_localAddr = SocketAddress();
	_remoteAddr = SocketAddress();
	_request = HttpRequest();
	_response = HttpResponse();
	_responseBuffer = std::deque<HttpResponse>();
	long pageSize = sysconf(_SC_PAGESIZE);
	if (pageSize == -1)
	{
		// handle error: fallback, throw, or use a default
		perror("sysconf");
		pageSize = 4096; // safe default on most systems
	}
	_receiveBuffer = std::vector<char>(static_cast<size_t>(pageSize)); // Is about 4KB depending on the system
	_potentialServers = NULL;
	_state = CLIENT_WAITING_FOR_REQUEST;
	_lastActivity = time(NULL);
}

Client::Client(const Client &src)
{
	*this = src;
}

Client::Client(FileDescriptor socketFd, SocketAddress clientAddr, SocketAddress remoteAddr)
{
	_clientFd = socketFd;
	_localAddr = clientAddr;
	_remoteAddr = remoteAddr;
	_request = HttpRequest();
	_response = HttpResponse();
	_responseBuffer = std::deque<HttpResponse>();
	long pageSize = sysconf(_SC_PAGESIZE);
	if (pageSize == -1)
	{
		perror("sysconf");
		pageSize = 4096;
	}
	_receiveBuffer = std::vector<char>(static_cast<size_t>(pageSize));
	_potentialServers = NULL;
	_state = CLIENT_WAITING_FOR_REQUEST;
	_lastActivity = time(NULL);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Client::~Client()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Client &Client::operator=(const Client &rhs)
{
	if (this != &rhs)
	{
		_clientFd = rhs._clientFd;
		_localAddr = rhs._localAddr;
		_remoteAddr = rhs._remoteAddr;
		_request = rhs._request;
		_response = rhs._response;
		_responseBuffer = rhs._responseBuffer;
		_receiveBuffer = rhs._receiveBuffer;
		_holdingBuffer = rhs._holdingBuffer;
		_potentialServers = rhs._potentialServers;
		_state = rhs._state;
		_lastActivity = rhs._lastActivity;
		_keepAlive = rhs._keepAlive;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Client::handleEvent(epoll_event event)
{
	if (event.events & EPOLLIN)
	{
		_handleBuffer();
	}
	else if (event.events & EPOLLOUT)
	{
		_handleResponseBuffer();
	}
	else if (event.events & (EPOLLERR | EPOLLHUP))
	{
		_state = CLIENT_DISCONNECTED;
	}
}

void Client::_handleBuffer()
{
	Logger::debug("Client: Reading from socket fd: " + StrUtils::toString(_clientFd.getFd()));
	ssize_t bytesRead = recv(_clientFd.getFd(), &_receiveBuffer[0], _receiveBuffer.size(), 0);
	Logger::debug("Client: Read " + StrUtils::toString(bytesRead) + " bytes");
	
	if (bytesRead <= 0)
	{
		Logger::debug("Client: Connection closed or error, disconnecting");
		_state = CLIENT_DISCONNECTED;
		return;
	}
	
	_holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(), _receiveBuffer.begin() + bytesRead);
	Logger::debug("Client: Total buffer size: " + StrUtils::toString(_holdingBuffer.size()));
	
	// Debug: Print the raw request
	std::string rawRequest(_holdingBuffer.begin(), _holdingBuffer.end());
	Logger::debug("Client: Raw request: " + rawRequest);
	
	_handleRequest();
}

void Client::_handleRequest()
{
	Logger::debug("Client: Handling request, buffer size: " + StrUtils::toString(_holdingBuffer.size()));
	
	// Set the server for this request before parsing
	if (_potentialServers && !_potentialServers->empty())
	{
		Logger::debug("Client: Setting server from potential servers, count: " + StrUtils::toString(_potentialServers->size()));
		Server *serverPtr = const_cast<Server*>(&_potentialServers->front());
		std::stringstream ss;
		ss << "Client: Server pointer: " << serverPtr;
		Logger::debug(ss.str());
		_request.setServer(serverPtr);
	}
	else
	{
		_response.setStatus(500, "Internal Server Error");
		_response.setBody(NULL, NULL);
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	
	// Parse the HTTP request from the buffer
	HttpRequest::ParseState parseState = _request.parseBuffer(_holdingBuffer, _response);
	Logger::debug("Client: Parse state: " + StrUtils::toString(parseState));
	
	// If parsing is not complete, wait for more data
	if (parseState != HttpRequest::PARSING_COMPLETE)
	{
		if (parseState == HttpRequest::PARSING_ERROR)
		{
			_state = CLIENT_DISCONNECTED;
		}
		return;
	}
	
	// Identify if request is handled by the server then route
	Logger::debug("Client: Getting server for request");
	Server *server = _request.getServer();
	if (!server)
	{
		Logger::error("Client: No server available for request");
		_response.setStatus(500, "Internal Server Error");
		_response.setBody(NULL, NULL);
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	
	Logger::debug("Client: Got server, getting location for URI: " + _request.getUri());
	const Location *location = NULL;
	try
	{
		location = server->getLocation(_request.getUri());
		Logger::debug("Client: Location lookup complete");
	}
	catch (const std::exception &e)
	{
		Logger::error("Client: Exception during location lookup: " + std::string(e.what()));
		_response.setStatus(500, "Internal Server Error");
		_response.setBody(NULL, NULL);
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	catch (...)
	{
		Logger::error("Client: Unknown exception during location lookup");
		_response.setStatus(500, "Internal Server Error");
		_response.setBody(NULL, NULL);
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	if (!location) // 1. Verify location can be found on server (returns Null if exact match / longest prefix match is not found)
	{
		_response.setStatus(404, "Not Found");
		_response.setBody(NULL, _request.getServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	else if (std::find(location->getAllowedMethods().begin(), location->getAllowedMethods().end(),
					   _request.getMethod()) == location->getAllowedMethods().end()) // 2. Verify method is allowed
	{
		_response.setStatus(405, "Method Not Allowed");
		_response.setBody(location, _request.getServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}

	// 3. Once location is found sanitize the request (can only be done after location is found)
	_request.sanitizeRequest(_response, _request.getServer(), location);
	
	// Use method handlers
	IMethodHandler *handler = MethodHandlerFactory::createHandler(_request.getMethod());
	if (handler)
	{
		handler->handleRequest(_request, _response, _request.getServer(), location);
		delete handler;
	}
	else
	{
		_response.setStatus(405, "Method Not Allowed");
		_response.setBody(location, _request.getServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
	}
	
	// Add response to response buffer and prepare for sending
	_responseBuffer.push_back(_response);
	_state = CLIENT_PROCESSING_RESPONSES;
}

void Client::_handleResponseBuffer()
{
	if (_responseBuffer.empty())
	{
		_state = CLIENT_DISCONNECTED;
		return;
	}
	
	HttpResponse &response = _responseBuffer.front();
	response.sendResponse(_clientFd);
	_responseBuffer.pop_front();
	
	if (_responseBuffer.empty())
	{
		_state = CLIENT_DISCONNECTED;
	}
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

Client::ClientState Client::getCurrentState() const
{
	return _state;
}

void Client::setState(ClientState newState)
{
	_state = newState;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

int Client::getSocketFd() const
{
	return _clientFd.getFd();
}

const SocketAddress &Client::getLocalAddr() const
{
	return _localAddr;
}

const SocketAddress &Client::getRemoteAddr() const
{
	return _remoteAddr;
}

const std::vector<Server> &Client::getPotentialServers() const
{
	return *_potentialServers;
}

void Client::setPotentialServers(const std::vector<Server> &potentialServers)
{
	_potentialServers = &potentialServers;
}

bool Client::isTimedOut() const
{
	time_t currentTime = time(NULL);
	return (currentTime - _lastActivity) > 30; // 30 second timeout
}