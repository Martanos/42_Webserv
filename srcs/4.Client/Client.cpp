#include "../../includes/Client.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/PerformanceMonitor.hpp"
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
		// handle error: fallback, throw, or use a default
		perror("sysconf");
		pageSize = 4096; // safe default on most systems
	}
	_receiveBuffer = std::vector<char>(static_cast<size_t>(pageSize)); // Is about 4KB depending on the system
	_holdingBuffer = std::vector<char>();
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

Client &Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_clientFd = rhs._clientFd;
		_localAddr = rhs._localAddr;
		_remoteAddr = rhs._remoteAddr;
		_state = rhs._state;
		_request = rhs._request;
		_response = rhs._response;
		_responseBuffer = rhs._responseBuffer;
		_receiveBuffer = rhs._receiveBuffer;
		_holdingBuffer = rhs._holdingBuffer;
		_potentialServers = rhs._potentialServers;
		_lastActivity = rhs._lastActivity;
		_keepAlive = rhs._keepAlive;
	}
	return *this;
}

/*
** --------------------------------- EVENT HANDLING
*----------------------------------
*/

// Main event handling method delegates
// to appropriate methods based on the current state and event type
void Client::handleEvent(epoll_event event)
{
	// Update the last activity time
	updateActivity();
	switch (event.events)
	{
	case EPOLLIN:
	case EPOLLOUT:
	{
		switch (_state)
		{
		case CLIENT_WAITING_FOR_REQUEST:
		case CLIENT_PROCESSING_REQUESTS:
		{
			_handleBuffer();
			break;
		}
		case CLIENT_PROCESSING_RESPONSES:
		{
			_handleResponseBuffer();
			break;
		}
		default:
			break;
		}
	}
	case EPOLLHUP:
	case EPOLLRDHUP:
	case EPOLLERR:
	{
		std::stringstream ss;
		ss << "Client disconnected: " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_state = CLIENT_DISCONNECTED;
		break;
	}
	default:
	{
		std::stringstream ss;
		ss << "Unknown event: " << event.events << " for client " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_state = CLIENT_DISCONNECTED;
		break;
	}
	}
}

/*
** --------------------------------- REQUEST PROCESSING METHODS
*----------------------------------
*/

// Facilitates parsing of complete requests in the holding buffer
void Client::_handleBuffer()
{
	// Stage 1: Ingest data from the client (recv overwrites all data each time)
	_state = CLIENT_PROCESSING_REQUESTS;
	ssize_t bytesRead = recv(_clientFd.getFd(), _receiveBuffer.data(), _receiveBuffer.size(), 0);
	if (bytesRead <= 0)
	{
		// Client disconnected or error occurred trigger immediate disconnect
		std::stringstream ss;
		ss << "Client disconnected during read: " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_state = CLIENT_DISCONNECTED;
		return;
	}

	// Stage 2: append data to the holding buffer
	_holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(), _receiveBuffer.begin() + bytesRead);

	// Stage 3: parse the request
	size_t previousSize = 0;
	while (previousSize != _holdingBuffer.size())
	{
		previousSize = _holdingBuffer.size();
		_request.parseBuffer(_holdingBuffer, _response);
		// Each loop we check the parse state and handle the appropriate action
		switch (_request.getParseState())
		{ // TODO: include a waiting for server info state
		case HttpRequest::PARSING_BODY:
		{
			// In this block we faciliate things that require
			// Information from URI and headers to determine
			// due to the nature of virtual servers
			_identifyServer();
			// TODO: Retroactive functions (EG: URI and header size restrictions)
			break;
		}
		case HttpRequest::PARSING_COMPLETE:
		{
			// Pass the completed request to the router
			_handleRequest();
			// Reset the object to be ready for the next request
			_request.reset();
			// Response shouldn't have been modified at this point
			break;
		}
		case HttpRequest::PARSING_ERROR:
		{
			// If a parsing error occurs immdiately queue an error response
			// Don't waste resources processing subsequent requests
			// Clear request caches and buffers as well to save space
			_request.reset();
			_responseBuffer.push_back(_response);
			_response.reset();
			_state = CLIENT_PROCESSING_RESPONSES;
			break;
		}
		default:
			break;
		}
	}
	if (_responseBuffer.empty())
	{
		_state = CLIENT_WAITING_FOR_REQUEST;
	}
	else
	{
		_state = CLIENT_PROCESSING_RESPONSES;
	}
}

void Client::_handleRequest()
{
	// Identify if request is handled by the server then route
	// 1. Verify location can be found on server (returns root if longest prefix match is not found)
	// TODO: remove already inlined method handler factory
	const Location *location = _request.getServer()->getLocation(_request.getUri());
	// 2. Verify method is allowed
	if (location->getAllowedMethods().find(_request.getMethod()) == location->getAllowedMethods().end())
	{
		_response.setStatus(405, "Method Not Allowed");
		_response.setBody(_request.getServer()->getStatusPage(405));
		_response.setHeader("Content-Type", "text/html");
		_response.setHeader("Content-Length", StringUtils::toString(_response.getBody().length()));
		_response.setHeader("Allow", "GET, HEAD");
		return;
	}

	// 3. Once location is found sanitize the request
	_request.sanitizeRequest(_response, _request.getServer(), location);
	if (_request.getParseState() == HttpRequest::PARSING_ERROR)
	{
		_responseBuffer.push_back(_response);
		_response.reset();
		_state = CLIENT_PROCESSING_RESPONSES;
		return;
	}

	// TODO: Implement new static method handler factory
	MethodHandlerFactory::getInstance()
		.getHandler(_request.getMethod())
		->handle(_request, _response, _request.getServer(), location);
}

/*
** --------------------------------- RESPONSE PROCESSING METHODS
*----------------------------------
*/
// TODO: Refine this
// This facilitates processing of responses in response queue
void Client::_flushResponseQueue()
{
	bool canContinue = true;
	while (canContinue)
	{
		canContinue = _responseBuffer.front().sendResponse(_clientFd);
		_responseBuffer.pop_front();
	}
}

// TODO: Refactor this to change behaviour depending on response states
void Client::_sendResponse()
{
	if (_response.getRawResponse().empty())
	{
		// Generate the raw HTTP response if not already done
		_response.setRawResponse(_response.toString());
	}

	std::string responseData = _response.getRawResponse();
	size_t totalSize = responseData.length();
	size_t remainingBytes = totalSize - _response.getBytesSent();

	if (remainingBytes == 0)
	{
		// Response fully sent

		// Check if we should keep the connection alive
		bool clientWantsKeepAlive = false;
		const std::vector<std::string> &connectionHeaders = _request.getHeader("Connection");

		for (size_t i = 0; i < connectionHeaders.size(); ++i)
		{
			std::string header = connectionHeaders[i];
			// Convert to lowercase for comparison
			std::transform(header.begin(), header.end(), header.begin(), ::tolower);

			if (header.find("keep-alive") != std::string::npos)
			{
				clientWantsKeepAlive = true;
			}
			else if (header.find("close") != std::string::npos)
			{
				clientWantsKeepAlive = false;
				break;
			}
		}

		// Server configuration also matters
		bool serverAllowsKeepAlive = _request.getServer()->getKeepAlive();

		// HTTP/1.0 defaults to close, HTTP/1.1 defaults to keep-alive
		bool defaultKeepAlive = (_request.getVersion() == "HTTP/1.1");

		_keepAlive = serverAllowsKeepAlive && (clientWantsKeepAlive || (defaultKeepAlive && connectionHeaders.empty()));

		if (_keepAlive)
		{
			// Reset for next request on same connection
			_request.reset();
			_response = HttpResponse(); // Create fresh response
			_state = CLIENT_WAITING_FOR_REQUEST;
			updateActivity(); // Reset timeout

			// Important: Set response header to inform client
			_response.setHeader("Connection", "keep-alive");
			// Optionally set Keep-Alive timeout
			_response.setHeader("Keep-Alive", "timeout=15, max=100");
		}
		else
		{
			_state = CLIENT_DISCONNECTED;
			_response.setHeader("Connection", "close");
		}
	}

	// Send remaining data
	ssize_t bytesSent = _clientFd.sendData(responseData);

	if (bytesSent > 0)
	{
		_response.setBytesSent(_response.getBytesSent() + bytesSent);

		// Log progress for large responses
		if (totalSize > static_cast<size_t>(sysconf(_SC_PAGE_SIZE)))
		{
			std::stringstream ss;
			ss << "Sent " << _response.getBytesSent() << "/" << totalSize << " bytes to client " << _clientFd.getFd();
			Logger::log(Logger::DEBUG, ss.str());
		}
	}
	else if (bytesSent == 0)
	{
		// Connection closed by client
		Logger::log(Logger::INFO, "Client closed connection during response");
		throw std::runtime_error("Client closed connection");
	}
	else
	{
		// TODO: Checking errno check if this is allowed
		// Error occurred
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// Socket buffer full, try again later
			return;
		}
		else
		{
			std::stringstream ss;
			ss << "Send error: " << strerror(errno);
			Logger::log(Logger::ERROR, ss.str());
			throw std::runtime_error(ss.str());
		}
	}
}

/*
** --------------------------------- EVENT HANDLING
*----------------------------------
*/

// TODO: Refactor this for smarter default port handling
void Client::_identifyServer()
{
	// If no server can be identified we set response server to NULL
	if (!_potentialServers || _potentialServers->empty() || _request.getHeader("host").empty())
	{
		return;
	}
	// First parse the host header into name and port
	std::string hostHeaders = _request.getHeader("host")[0];
	std::string hostName;
	std::string hostPort;
	if (hostHeaders.find(':') != std::string::npos)
	{
		hostName = hostHeaders.substr(0, hostHeaders.find(':'));
		hostPort = hostHeaders.substr(hostHeaders.find(':') + 1);
	}
	else
	{
		hostName = hostHeaders;
		hostPort = "80"; // HTTP default port
	}

	// Determine if the port is the same as current server port
	std::stringstream ss;
	ss << _request.getServer()->getPort();
	if (hostPort != ss.str())
	{
		return;
	}

	// If host header is present, attempt to match host name to a server
	// configuration
	if (!hostHeaders.empty())
	{
		for (std::vector<Server>::const_iterator it = _potentialServers->begin(); it != _potentialServers->end(); ++it)
		{
			if (it->getHost() == hostName)
			{
				_request.setServer(const_cast<Server *>(&(*it)));
				break;
			}
		}
	}
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
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

// TODO: Refactor this for smarter and more accurate timeout handling
bool Client::isTimedOut() const
{
	const time_t TIMEOUT_SECONDS = 30;
	return (time(NULL) - _lastActivity) > TIMEOUT_SECONDS;
}

void Client::setPotentialServers(const std::vector<Server> &potentialServers)
{
	_potentialServers = const_cast<std::vector<Server> *>(&potentialServers);
}
