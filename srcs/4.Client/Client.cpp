#include "Client.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Client::Client()
{
	_currentState = CLIENT_WAITING_FOR_REQUEST;
	_server = NULL;
	_keepAlive = true;
	_lastActivity = time(NULL);
	_readBuffer = "";
}

Client::Client(const Client &src)
{
	*this = src;
}

Client::Client(FileDescriptor socketFd, SocketAddress clientAddr)
{
	_clientFd = socketFd;
	_clientAddr = clientAddr;
	_currentState = CLIENT_WAITING_FOR_REQUEST;
	_server = NULL;
	_keepAlive = true;
	_lastActivity = time(NULL);
	_readBuffer = "";
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Client::~Client()
{
	// TODO: Automatic failure messages based on state
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Client &Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_clientFd = rhs._clientFd;
		_clientAddr = rhs._clientAddr;
		_currentState = rhs._currentState;
		_request = rhs._request;
		_response = rhs._response;
		_readBuffer = rhs._readBuffer;
		_lastActivity = rhs._lastActivity;
		_keepAlive = rhs._keepAlive;
		_server = rhs._server;
	}
	return *this;
}

/*
** --------------------------------- EVENT HANDLING ----------------------------------
*/

// TODO: replace throw with custom exception
void Client::handleEvent(epoll_event event)
{
	updateActivity();

	if (event.events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
	{
		std::stringstream ss;
		ss << "Client disconnected: " << _clientFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		throw std::runtime_error(ss.str());
	}

	switch (_currentState)
	{
	case CLIENT_WAITING_FOR_REQUEST:
		if (event.events & EPOLLIN)
		{
			_currentState = CLIENT_READING_REQUEST;
			readRequest();
		}
		break;
	case CLIENT_READING_REQUEST:
		if (event.events & EPOLLIN)
			readRequest();
		break;
	case CLIENT_PROCESSING_REQUEST:
		// This state should transition automatically in readRequest()
		break;
	case CLIENT_SENDING_RESPONSE:
		if (event.events & EPOLLOUT)
		{
			sendResponse();
		}
		break;
	case CLIENT_READING_FILE:
		if (event.events & EPOLLIN)
		{
			readFileChunk();
		}
		break;
	case CLIENT_WRITING_FILE:
		if (event.events & EPOLLOUT)
		{
			sendResponse();
		}
		break;
	}
}

void Client::readRequest()
{
	std::string buffer;
	ssize_t bytesRead = _clientFd.receiveData(buffer);

	if (bytesRead < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			_currentState = CLIENT_WAITING_FOR_REQUEST;
			return;
		}
		std::stringstream ss;
		ss << "Error reading from client: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error("Read error");
	}

	// Null-terminate and add to buffer
	buffer[bytesRead] = '\0';
	_readBuffer.append(buffer, bytesRead);

	// Parse the request
	HttpRequest::ParseState parseResult = _request.parseBuffer(_readBuffer, _readBuffer.size());

	switch (parseResult)
	{
	case HttpRequest::PARSE_COMPLETE:
		_currentState = CLIENT_PROCESSING_REQUEST;
		_processHTTPRequest();
		_currentState = CLIENT_SENDING_RESPONSE;
		break;
	case HttpRequest::PARSE_ERROR:
		_generateErrorResponse(400, "Bad Request");
		_currentState = CLIENT_SENDING_RESPONSE;
		break;

	case HttpRequest::PARSE_REQUEST_LINE:
	case HttpRequest::PARSE_HEADERS:
	case HttpRequest::PARSE_BODY:
	{
		_identifyServer();
		_currentState = CLIENT_READING_REQUEST;
		// Continue reading - need more data
		break;
	}
	}
}

void Client::sendResponse()
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
		if (_keepAlive)
		{
			// Reset for next request
			_request.reset();
			_response.reset();
			_readBuffer.clear();
			_currentState = CLIENT_WAITING_FOR_REQUEST;
		}
		else
		{
			// Close connection
			throw std::runtime_error("Response complete - close connection");
		}
		return;
	}

	// Send remaining data
	const char *dataPtr = responseData.c_str() + _response.getBytesSent();
	ssize_t bytesSent = _clientFd.sendData(responseData);

	if (bytesSent > 0)
	{
		_response.setBytesSent(_response.getBytesSent() + bytesSent);

		// Log progress for large responses
		if (totalSize > sysconf(_SC_PAGE_SIZE))
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
** --------------------------------- PRIVATE METHODS ----------------------------------
*/

void Client::_identifyServer()
{
	if (_server != NULL)
	{
		return;
	}
	// If host header is not present, use the first server in the potential servers
	if (_request.getHeader("host").empty())
	{
		_server = &(_potentialServers[0]);
	}
	else
	{
		// If host header is present, attempt to match host name to a server configuration
		for (std::vector<Server>::iterator it = _potentialServers.begin(); it != _potentialServers.end(); ++it)
		{
			if (it->getHost() == _request.getHeader("host"))
			{
				_server = &(*it);
				break;
			}
		}
		// If no server is found, use the first server in the potential servers
		if (_server == NULL)
		{
			_server = &(_potentialServers[0]);
		}
	}
}

void Client::_processHTTPRequest()
{
	// Create router instance
	RequestRouter router;

	// Clear any previous response
	_response.reset();

	// Route the request through the handler system
	router.route(_request, _response, _server);

	// Check Connection header to determine keep-alive
	std::string connectionHeader = _request.getHeader("connection");

	// HTTP/1.1 defaults to keep-alive, HTTP/1.0 defaults to close
	if (_request.getVersion() == "HTTP/1.1")
	{
		_keepAlive = (connectionHeader != "close");
	}
	else
	{
		_keepAlive = (connectionHeader == "keep-alive");
	}

	// Override with server configuration if needed
	if (_server && !_server->getKeepAlive())
	{
		_keepAlive = false;
	}

	// Ensure Connection header is set in response
	if (!_keepAlive)
	{
		_response.setHeader("Connection", "close");
	}

	// Log the response
	std::stringstream logMsg;
	logMsg << "Response: " << _response.getStatusCode()
		   << " " << _response.getStatusMessage()
		   << " for " << _request.getMethod()
		   << " " << _request.getUri();
	Logger::log(Logger::INFO, logMsg.str());

	// Transition to sending state
	_currentState = CLIENT_SENDING_RESPONSE;
}

void Client::_generateErrorResponse(int statusCode, const std::string &message)
{
	// This can be simplified to just set basic error response
	// The router handles most error cases now
	_response.reset();
	_response.setStatus(statusCode, message);
	_response.setBody(DefaultStatusMap::getStatusBody(statusCode));
	_response.setHeader("Content-Type", "text/html");
	_response.setHeader("Content-Length",
						StringUtils::toString(_response.getBody().length()));
	_response.setHeader("Connection", _keepAlive ? "keep-alive" : "close");
	_response.setHeader("Server", "42_Webserv/1.0");
}

/*
** --------------------------------- ACCESSOR METHODS ----------------------------------
*/

Client::State Client::getCurrentState() const
{
	return _currentState;
}

void Client::setState(State newState)
{
	_currentState = newState;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

int Client::getSocketFd() const
{
	return _clientFd.getFd();
}

const std::string &Client::getClientIP() const
{
	return _clientAddr.getHostString();
}

const Server *Client::getServer() const
{
	return _server;
}

void Client::setServer(const Server *server)
{
	_server = const_cast<Server *>(server);
}

bool Client::isTimedOut() const
{
	const time_t TIMEOUT_SECONDS = 30;
	return (time(NULL) - _lastActivity) > TIMEOUT_SECONDS;
}

const std::vector<Server> &Client::getPotentialServers() const
{
	return _potentialServers;
}

void Client::setPotentialServers(const std::vector<Server> &potentialServers) const
{
	_potentialServers = potentialServers;
}
