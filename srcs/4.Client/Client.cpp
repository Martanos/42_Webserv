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
	_state = WAITING_FOR_EPOLLIN;
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
	_state = WAITING_FOR_EPOLLIN;
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
	switch (event.events)
	{
	case EPOLLIN:
		_handleBuffer();
		break;
	case EPOLLOUT:
		_handleResponseBuffer();
		break;
	case EPOLLERR | EPOLLHUP:
		_state = DISCONNECTED;
		break;
	default:
		Logger::debug("Client: Unknown event: " + StrUtils::toString(event.events));
		_state = DISCONNECTED;
		break;
	}
	updateActivity(); // base last activity time off of when event handled
}

void Client::_handleBuffer()
{
	// Ingest up to 4096 bytes of data from the incoming socket buffer
	ssize_t bytesRead = recv(_clientFd.getFd(), &_receiveBuffer[0], _receiveBuffer.size(), 0);
	if (bytesRead <= 0)
	{
		Logger::warning("Client: " + _remoteAddress.getHostString() + ":" + _remoteAddress.getPortString() +
						" disconnected or error occurred : " + std::string(strerror(errno)));
		_state = DISCONNECTED;
		return;
	}
	// Attach new data to the holding buffer
	_holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(), _receiveBuffer.begin() + bytesRead);
	// Debug: Print the raw request
	Logger::debug("Client: " + _remoteAddress.getHostString() + ":" + _remoteAddress.getPortString() +
				  " current buffer: " + std::string(_holdingBuffer.begin(), _holdingBuffer.end()));

	// Parse the request
	_handleRequest();
}

void Client::_handleRequest()
{
	while (!_holdingBuffer.empty())
	{
		// Set/refresh current potential servers if not set for the request yet
		if (_request.getPotentialServers() == NULL)
			_request.setPotentialServers(_potentialServers);
		HttpRequest::ParseState parseState = _request.parseBuffer(_holdingBuffer, _response);
		switch (parseState)
		{
		case HttpRequest::PARSING_COMPLETE:
			_routeRequest();
			_responseBuffer.push_back(_response);
			_response.reset();
			_request.reset();
			// Set state to waiting for epollout here as we know we have responses ready
			_state = WAITING_FOR_EPOLLOUT;
			break;
		case HttpRequest::PARSING_ERROR:
			// Any errors here are considered fatal and denote an immediate disconnect
			_responseBuffer.push_back(_response);
			_state = WAITING_FOR_EPOLLOUT;
			return;
		case HttpRequest::PARSING_URI:
		case HttpRequest::PARSING_HEADERS:
		case HttpRequest::PARSING_BODY:
			// Ending on any of these states means we need more data to complete the request
			return;
		default:
			break;
		}
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
		return;
	}
	if (!location) // 1. Verify location can be found on server (returns Null if exact match / longest prefix match is
				   // not found)
	{
		_response.setStatus(404, "Not Found");
		_response.setBody(NULL, _request.getSelectedServer());
		_response.setHeader(Header("Content-Type: text/html"));
		_response.setHeader(Header("Content-Length: " + StrUtils::toString(_response.getBody().length())));
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
}

// Write up to 4096 worth of response to the client each time this is called
// Highlevel consideration is to flush the response buffer so EPOLLOUT takes priority over EPOLLIN
void Client::_handleResponseBuffer()
{
	errno = 0;
	ssize_t totalBytesSent = 0;
	// SafeGuard should never occur
	if (_responseBuffer.empty())
	{
		Logger::error("Client: Response buffer is empty while handling response buffer for client: " +
						  _remoteAddress.getHostString() + ":" + _remoteAddress.getPortString(),
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_state = WAITING_FOR_EPOLLIN;
		return;
	}
	HttpResponse &response = _responseBuffer.front();
	while (totalBytesSent < HTTP::DEFAULT_SEND_SIZE && !_responseBuffer.empty())
	{
		response.sendResponse(_clientFd, totalBytesSent);
		switch (response.getSendingState())
		{
		case HttpResponse::RESPONSE_SENDING_COMPLETE:
		{
			switch (response.getResponseType())
			{
			case HttpResponse::SUCCESS:
			case HttpResponse::ERROR:
				_responseBuffer.pop_front();  // Clear response from buffer when its done
				if (!_responseBuffer.empty()) // If the response buffer is not empty set the next response to send
				{
					response = _responseBuffer.front();
					_state = WAITING_FOR_EPOLLOUT;
				}
				else
					_state = WAITING_FOR_EPOLLIN; // else ready to continue processing data
				if (!_keepAlive)
					_state = DISCONNECTED; // If keep alive is false however then we disconnect the client
				break;
			case HttpResponse::FATAL_ERROR:
				_state = DISCONNECTED;
				return;
			}
			break;
		}
		case HttpResponse::RESPONSE_SENDING_ERROR: // Fatal error encountered sending the response immediately
												   // disconnect the client
			Logger::error(
				"Client: Fatal error encountered while sending response for client: " + _remoteAddress.getHostString() +
					":" + _remoteAddress.getPortString() + ": " + strerror(errno),
				__FILE__, __LINE__, __PRETTY_FUNCTION__);
			_state = DISCONNECTED;
			return;
		case HttpResponse::RESPONSE_SENDING_MESSAGE:
		case HttpResponse::RESPONSE_SENDING_BODY:
		{
			// In the middle of sending the response only ends here if 4096 bytes where reached
			_state = WAITING_FOR_EPOLLOUT;
			break;
		}
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