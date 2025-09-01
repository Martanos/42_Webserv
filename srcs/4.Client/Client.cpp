/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: malee <malee@student.42singapore.sg>       +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/01 17:12:02 by malee             #+#    #+#             */
/*   Updated: 2025/09/01 17:56:51 by malee            ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
		_socketFd.closeDescriptor();
		throw std::runtime_error("Client disconnected");
	}

	if (event.events & EPOLLIN)
	{
		readRequest();
	}

	if (event.events & EPOLLOUT)
	{
		sendResponse();
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
	if (_currentState != CLIENT_SENDING_RESPONSE)
	{
		return;
	}

	// Generate response if not already done
	if (_response.isEmpty()) // You'll need to add this method to HttpResponse
	{
		_generateErrorResponse(500, "Internal Server Error");
	}
	std::stringstream ss;
	ss << _response.toString();
	std::string responseData = ss.str(); // You'll need to add this method

	ssize_t bytesSent = send(_socketFd.getFd(), responseData.c_str(), responseData.length(), 0);

	if (bytesSent < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
		{
			return; // Try again later
		}
		throw std::runtime_error("Send error");
	}

	// For simplicity, assuming we send the complete response at once
	// In a real implementation, you'd track partial sends
	if (static_cast<size_t>(bytesSent) == responseData.length())
	{
		if (_keepAlive)
		{
			// Reset for next request
			_request.reset();
			_response.reset(); // You'll need to add this method
			_readBuffer.clear();
			_currentState = CLIENT_WAITING_FOR_REQUEST;
		}
		else
		{
			// Close connection
			throw std::runtime_error("Connection should be closed");
		}
	}
}

/*
** --------------------------------- PRIVATE METHODS ----------------------------------
*/

void Client::_processHTTPRequest()
{
	// Basic request processing
	std::string method = _request.getMethod();
	std::string uri = _request.getUri();

	if (method == "GET")
	{
		_handleGetRequest();
	}
	else if (method == "POST")
	{
		_handlePostRequest();
	}
	else if (method == "DELETE")
	{
		_handleDeleteRequest();
	}
	else
	{
		_generateErrorResponse(405, "Method Not Allowed");
	}
}

void Client::_generateErrorResponse(int statusCode, const std::string &message)
{
	_response.reset();
	_response.setStatus(statusCode, message.empty() ? DefaultStatusMap::getStatusMessage(statusCode) : message);
	_response.setHeader("Content-Type", "text/html");
	_response.setHeader("Connection", _keepAlive ? "keep-alive" : "close");

	// Get error page from server config or use default
	std::string errorBody;
	if (_server)
	{
		errorBody = _server->getStatusPage(statusCode);
	}
	if (errorBody.empty())
	{
		errorBody = DefaultStatusMap::getStatusBody(statusCode);
	}

	_response.setBody(errorBody);
	_response.setHeader("Content-Length", StringUtils::toString(errorBody.length()));
}

void Client::_generateFileResponse(const std::string &filePath)
{
	// Simplified file response
	_generateErrorResponse(501, "Not Implemented");
}

void Client::_generateDirectoryListing(const std::string &dirPath)
{
	// Simplified directory listing
	_generateErrorResponse(501, "Not Implemented");
}

void Client::_handleCGIRequest()
{
	// Simplified CGI handling
	_generateErrorResponse(501, "Not Implemented");
}

void Client::_handleFileUpload()
{
	// Simplified file upload
	_generateErrorResponse(501, "Not Implemented");
}

bool Client::_isMethodAllowed(const std::string &method) const
{
	// This would check against server/location configuration
	// For now, allow basic methods
	return (method == "GET" || method == "POST" || method == "DELETE");
}

std::string Client::_resolveFilePath(const std::string &uri) const
{
	if (_server)
	{
		return _server->getRoot() + uri;
	}
	return "www" + uri; // Default fallback
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
	_server = server;
}

bool Client::isTimedOut() const
{
	const time_t TIMEOUT_SECONDS = 30;
	return (time(NULL) - _lastActivity) > TIMEOUT_SECONDS;
}
