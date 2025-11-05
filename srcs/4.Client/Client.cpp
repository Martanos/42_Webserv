#include "../../includes/Core/Client.hpp"
#include "../../includes/Core/GetHandler.hpp"
#include "../../includes/Core/MethodHandlerFactory.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HTTP.hpp"
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
	_remoteAddress = SocketAddress();
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

Client::Client(FileDescriptor socketFd, SocketAddress remoteAddress)
{
	_clientFd = socketFd;
	_remoteAddress = remoteAddress;
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
		_remoteAddress = rhs._remoteAddress;
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
	_response.reset();
	switch (event.events)
	{
	case EPOLLIN:
		_handleBuffer();
		break;
	case EPOLLOUT:
		_handleResponseBuffer();
		break;
	case EPOLLERR | EPOLLHUP:
		_state = CLIENT_DISCONNECTED;
		break;
	default:
		Logger::debug("Client: Unknown event: " + StrUtils::toString(event.events));
		_state = CLIENT_DISCONNECTED;
		break;
	}
	updateActivity(); // base last activity time off of when event handled
}

void Client::_handleBuffer()
{
	ssize_t bytesRead = recv(_clientFd.getFd(), &_receiveBuffer[0], _receiveBuffer.size(), 0);
	if (bytesRead <= 0)
	{
		Logger::warning("Client: " + _remoteAddress.getHostString() + ":" + _remoteAddress.getPortString() +
						" disconnected or error occurred : " + std::string(strerror(errno)));
		_state = CLIENT_DISCONNECTED;
		return;
	}
	_holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(), _receiveBuffer.begin() + bytesRead);
	// Debug: Print the raw request
	std::string rawRequest(_holdingBuffer.begin(), _holdingBuffer.end());
	Logger::debug("Client: " + _remoteAddress.getHostString() + ":" + _remoteAddress.getPortString() +
				  " current buffer: " + rawRequest);

	_handleRequest();
}

void Client::_handleRequest()
{
	// Refresh current potential servers
	if (_request.getPotentialServers() == NULL)
		_request.setPotentialServers(_potentialServers);
	HttpRequest::ParseState parseState = _request.parseBuffer(_holdingBuffer, _response);
	switch (parseState)
	{
	case HttpRequest::PARSING_COMPLETE:
		_routeRequest();
		break;
	case HttpRequest::PARSING_ERROR:
	{
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	default:
		break;
	}
}

void Client::_routeRequest()
{
	Logger::debug("Client: Got server, getting location for URI: " + _request.getUri());
	const Location *location = NULL;
	try
	{
		location = _request.getSelectedServer()->getLocation(_request.getUri());
		Logger::debug("Client: Matched location: " + location->getPath() + " for URI: " + _request.getUri());
	}
	catch (const std::exception &e)
	{
		Logger::error("Client: Exception during location lookup: " + std::string(e.what()) +
					  " for URI: " + _request.getUri());
		_response.setStatus(500, "Internal Server Error");
		_response.setBody(NULL, NULL);
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}
	if (!location) // 1. Verify location can be found on server (returns Null if exact match / longest prefix match is
				   // not found)
	{
		_response.setStatus(404, "Not Found");
		_response.setBody(NULL, _request.getSelectedServer());
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
		_response.setBody(location, _request.getSelectedServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
		_responseBuffer.push_back(_response);
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}

	// 3. Once location is found sanitize the request (can only be done after location is found)
	_request.sanitizeRequest(_response, _request.getSelectedServer(), location);

	// Use method handlers
	IMethodHandler *handler = MethodHandlerFactory::createHandler(_request.getMethod());
	if (handler)
	{
		Logger::debug("Client: Created handler for method: " + _request.getMethod());
		handler->handleRequest(_request, _response, _request.getSelectedServer(), location);
		delete handler;
	}
	else
	{
		Logger::error("Client: Failed to create handler for method: " + _request.getMethod());
		_response.setStatus(405, "Method Not Allowed");
		_response.setBody(location, _request.getSelectedServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
	}

	// Add response to response buffer and prepare for sending
	_responseBuffer.push_back(_response);
	_response
	_state = CLIENT_PROCESSING_RESPONSES;
}

// Write up to 4096 worth of response to the client each time this is called
void Client::_handleResponseBuffer()
{
	ssize_t totalBytesSent = 0;
	while (totalBytesSent < HTTP::DEFAULT_SEND_SIZE && !_responseBuffer.empty())
	{
		HttpResponse &response = _responseBuffer.front();
		response.sendResponse(_clientFd, totalBytesSent);
		switch (response.getState())
		{
		case HttpResponse::RESPONSE_SENDING_COMPLETE:
			Logger::debug("Client: Response sent completely for client: " + _remoteAddress.getHostString() + ":" +
						  _remoteAddress.getPortString());
			_responseBuffer.pop_front();
			if (_keepAlive)
				_state = CLIENT_WAITING_FOR_REQUEST;
			else
				_state = CLIENT_DISCONNECTED;
			break;
		case HttpResponse::RESPONSE_SENDING_ERROR:
			Logger::error("Client: Response sending error for client: " + _remoteAddress.getHostString() + ":" +
						  _remoteAddress.getPortString());
			_responseBuffer.pop_front();
			_state = CLIENT_DISCONNECTED;
			return;
		default:
			break;
		}
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

const SocketAddress &Client::getRemoteAddr() const
{
	return _remoteAddress;
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