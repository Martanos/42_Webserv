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
	_responseBuffer = std::deque<char>();
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
	// TODO: Log / send an error response depending on state of client
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
		_responseBuffer = rhs._responseBuffer;
		_receiveBuffer = rhs._receiveBuffer;
		_holdingBuffer = rhs._holdingBuffer;
		_potentialServers = rhs._potentialServers;
		_lastActivity = rhs._lastActivity;
	}
	return *this;
}

/*
** --------------------------------- EVENT HANDLING
*----------------------------------
*/

// This method is called by epoll manager when an EPOLLIN event is triggered
// Should empty the kernel buffer then proceed to handle the event
void Client::handleEvent(epoll_event event)
{
	// Update the last activity time
	updateActivity();
	if (event.events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
	{
		std::stringstream ss;
		ss << "Client disconnected: " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_state = CLIENT_DISCONNECTED;
		return;
	}
	switch (_state)
	{
	case CLIENT_WAITING_FOR_REQUEST:
	case CLIENT_READING_REQUEST:
	{
		if (event.events & EPOLLIN)
			_readRequest();
		break;
	}
	case CLIENT_SENDING_RESPONSE:
	{
		if (event.events & EPOLLOUT)
		{
			_sendResponse();
		}
		break;
	}
	default:
		break;
	}
}

//
void Client::_readRequest()
{
	// For each epoll event extract just 4096 due to level triggered nature of epoll we will basically be reading the
	// same data over and over again
	ssize_t bytesRead = recv(_clientFd.getFd(), _receiveBuffer.data(), _receiveBuffer.size(), 0);
	if (bytesRead <= 0)
	{
		// Client disconnected or error occurred
		// Trigger immediate disconnect
		std::stringstream ss;
		ss << "Client disconnected during read: " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_state = CLIENT_DISCONNECTED;
		return;
	}

	// After recieving the data append it to a holding buffer
	// We hold the data here so that we only extract relevant data to http request in case of a need to resets
	_holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(), _receiveBuffer.begin() + bytesRead);
	while (true)
	{
		size_t previousSize = _holdingBuffer.size();
		_request.parseBuffer(_holdingBuffer, _response);
		// Each loop we check the parse state and handle the appropriate action
		switch (_request.getParseState())
		{
		case HttpRequest::PARSING_COMPLETE:
		{
			_processHTTPRequest();
			_responseBuffer.push_back(_response);
			_request.reset();
			_response.reset();
			_state = CLIENT_SENDING_RESPONSE;
			break;
		}
		case HttpRequest::PARSING_ERROR:
		{
			_responseBuffer.push_back(_response);
			_state = CLIENT_SENDING_RESPONSE;
			break;
		}
		case HttpRequest::PARSING_BODY:
		{
			_identifyServer();
			break;
		}
		default:
			_state = CLIENT_READING_REQUEST;
			break;
		}
		// If size didn't change we break out of the loop theres no usable data to parse
		// This indicates we have parsed all complete requests
		if (_holdingBuffer.size() == previousSize)
		{
			break;
		}
	}
}

// TODO: Refine this
void Client::_flushResponseQueue()
{
	bool canContinue = true;
	while (canContinue)
	{
		canContinue = _responseBuffer.front().sendResponse(_clientFd);
		_responseBuffer.pop_front();
	}
}

// TODO: Refactor and move this to the response class
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
** --------------------------------- PRIVATE METHODS
*----------------------------------
*/

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
	if (hostPort != std::to_string(_request.getServer()->getPort()))
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

// TODO: Implement this
void Client::_identifyCGI()
{
}

void Client::_processHTTPRequest()
{
	// Create router instance
	RequestRouter router;

	// Route the request through the handler system
	router.route(_request, _response, _request.getServer());

	// Check Connection header to determine keep-alive
	const std::vector<std::string> &connectionHeaders = _request.getHeader("connection");
	std::string connectionHeader = connectionHeaders.empty() ? "" : connectionHeaders[0];

	// HTTP/1.1 defaults to keep-alive, HTTP/1.0 defaults to close
	if (_request.getVersion() == "HTTP/1.1")
	{
		_keepAlive = (connectionHeader != "close");
	}
	else
	{
		_keepAlive = (connectionHeader == "keep-alive");
	}

	// Ensure Connection header is set in response
	if (!_keepAlive)
	{
		_response.setHeader("Connection", "close");
	}

	// Log the response
	std::stringstream logMsg;
	logMsg << "Response: " << _response.getStatusCode() << " " << _response.getStatusMessage() << " for "
		   << _request.getMethod() << " " << _request.getUri();
	Logger::log(Logger::INFO, logMsg.str());
}

// TODO: move this to the http response class

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

Client::State Client::getCurrentState() const
{
	return _state;
}

void Client::setState(State newState)
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

bool Client::isTimedOut() const
{
	const time_t TIMEOUT_SECONDS = 30;
	return (time(NULL) - _lastActivity) > TIMEOUT_SECONDS;
}

const std::vector<Server> &Client::getPotentialServers() const
{
	return *_potentialServers;
}

void Client::setPotentialServers(const std::vector<Server> &potentialServers)
{
	_potentialServers = const_cast<std::vector<Server> *>(&potentialServers);
}
