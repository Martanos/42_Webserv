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
	_socketFd = socketFd;
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
	// Clean up resources based on current state if needed
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Client &Client::operator=(Client const &rhs)
{
	if (this != &rhs)
	{
		_socketFd = rhs._socketFd;
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

std::ostream &operator<<(std::ostream &o, Client const &i)
{
	o << "Client(fd=" << i.getSocketFd() << ", state=" << i.getCurrentState() << ")";
	return o;
}

/*
** --------------------------------- EVENT HANDLING ----------------------------------
*/

void Client::handleEvent(epoll_event event)
{
	updateActivity();

	if (event.events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR))
	{
		std::stringstream ss;
		ss << "Client disconnected: " << _socketFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		throw std::runtime_error("Client disconnected");
	}

	switch (_currentState)
	{
	case CLIENT_READING_REQUEST:
		if (event.events & EPOLLIN)
		{
			readRequest();
		}
		break;
	case CLIENT_READING_FILE:
		if (event.events & EPOLLIN)
		{
			readFileChunk();
		}
		break;
	case CLIENT_SENDING_RESPONSE:
		if (event.events & EPOLLOUT)
		{
			sendResponse();
		}
		break;
	}
}

void Client::readRequest()
{
	static const size_t BUFFER_SIZE = 4096;
	char buffer[BUFFER_SIZE];

	ssize_t bytesRead = recv(_socketFd.getFd(), buffer, BUFFER_SIZE - 1, 0);

	if (bytesRead < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			// No more data available right now
			return;
		}
		std::stringstream ss;
		ss << "Error reading from client: " << strerror(errno);
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error("Read error");
	}
	else if (bytesRead == 0)
	{
		// Client closed connection
		Logger::log(Logger::INFO, "Client closed connection");
		throw std::runtime_error("Client closed connection");
	}

	// Null-terminate and add to buffer
	buffer[bytesRead] = '\0';
	_readBuffer.append(buffer, bytesRead);

	// Parse the request
	HttpRequest::ParseState parseResult = _request.parseBuffer(_readBuffer);

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
		_currentState = CLIENT_READING_REQUEST;
		// Continue reading - need more data
		break;
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
	ssize_t bytesSent = send(_socketFd.getFd(), dataPtr, remainingBytes, MSG_NOSIGNAL);

	if (bytesSent > 0)
	{
		_response.setBytesSent(_response.getBytesSent() + bytesSent);

		// Log progress for large responses
		if (totalSize > 1024)
		{
			std::stringstream ss;
			ss << "Sent " << _response.getBytesSent() << "/" << totalSize << " bytes to client " << _socketFd.getFd();
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
** --------------------------------- FILE OPERATIONS ----------------------------------
*/

void startFileOperation(const std::string &filePath)
{
	// Open file in non-blocking mode
	int fd = open(filePath.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd == -1)
	{
		_fileOpState = FILE_OP_ERROR;
		return;
	}

	_fileOpFd.setFd(fd);
	_fileOpState = FILE_OP_READING;
	_fileOffset = 0;
	_responseBytesSent = 0;

	// Get file size
	struct stat fileStat;
	if (fstat(fd, &fileStat) == 0)
	{
		_fileSize = fileStat.st_size;
	}

	// Prepare response headers
	_response.setStatus(200, "OK");
	_response.setHeader("Content-Length", StringUtils::toString(_fileSize));
	_response.setHeader("Content-Type", getMimeType(filePath));

	// Switch to file reading state
	_currentState = CLIENT_READING_FILE;

	// Add file descriptor to epoll for reading
	// (In real implementation, this would be handled by EpollManager)
}

void readFileChunk()
{
	if (_fileOpState != FILE_OP_READING)
	{
		return;
	}

	// Try to read a chunk
	char buffer[FILE_CHUNK_SIZE];
	ssize_t bytesRead = read(_fileOpFd.getFd(), buffer, FILE_CHUNK_SIZE);

	if (bytesRead > 0)
	{
		// Successfully read data
		_fileBuffer.append(buffer, bytesRead);
		_fileOffset += bytesRead;

		// Check if we have enough data to send or reached end of file
		if (_fileBuffer.size() >= FILE_CHUNK_SIZE || _fileOffset >= _fileSize)
		{
			// Switch to sending mode
			_currentState = CLIENT_SENDING_RESPONSE;
			_fileOpState = FILE_OP_WRITING;

			// Modify epoll to watch for EPOLLOUT instead of EPOLLIN
			// _epollManager.modifyFd(_socketFd.getFd(), EPOLLOUT);
		}
	}
	else if (bytesRead == 0)
	{
		// EOF reached
		if (!_fileBuffer.empty())
		{
			_currentState = CLIENT_SENDING_RESPONSE;
			_fileOpState = FILE_OP_WRITING;
		}
		else
		{
			_fileOpState = FILE_OP_COMPLETE;
			finishFileOperation();
		}
	}
	else
	{
		// bytesRead < 0 - check if it's EAGAIN/EWOULDBLOCK
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			// No data available right now, will try again on next EPOLLIN
			return;
		}
		else
		{
			// Real error occurred
			_fileOpState = FILE_OP_ERROR;
			generateErrorResponse(500);
		}
	}
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
	return _socketFd.getFd();
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
