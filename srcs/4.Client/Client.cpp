#include "Client.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Client::Client()
{
}

Client::Client(const Client &src)
{
	*this = src;
}

Client::Client(FileDescriptor socketFd, SocketAddress clientAddr)
{
	_socketFd = socketFd;
	_clientAddr = clientAddr;
	_currentState = CLIENT_READING_REQUEST;
	_server = NULL;
	_keepAlive = true;
	_lastActivity = time(NULL);
	_readBuffer = "";
	_request = HttpRequest();
	_response = HttpResponse();
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Client::~Client()
{
	switch (_currentState)
	{
	case CLIENT_WAITING_FOR_REQUEST:
		break;
	case CLIENT_READING_REQUEST:
		break;
	}
	// TODO: Use current state to determine error response
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

// TODO: Implement this
std::ostream &operator<<(std::ostream &o, Client const &i)
{
	// o << "Value = " << i.getValue();
	return o;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Client::handleEvent(epoll_event event)
{
	switch (event.events)
	{
	case EPOLLIN:
		readRequest();
		break;
	case EPOLLOUT:
		sendResponse();
		break;
	case EPOLLHUP | EPOLLRDHUP | EPOLLERR | EPOLLPRI:
	{
		std::stringstream ss;
		ss << "Client disconnected: " << _socketFd.getFd();
		Logger::log(Logger::INFO, ss.str());
		_socketFd.closeDescriptor();
		throw std::runtime_error(ss.str());
	}
	default:
	{
		std::stringstream ss;
		ss << "Unknown event: " << event.events;
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	}
}

void Client::readRequest()
{
	// Check if client is in the correct state
	if (_currentState != CLIENT_READING_REQUEST && _currentState != CLIENT_WAITING_FOR_REQUEST)
	{
		std::stringstream ss;
		ss << "error reading request client expected state: CLIENT_READING_REQUEST"
		   << " current state: " << _currentState;
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	_socketFd.readFile(_readBuffer);
	if (_readBuffer.find("\r\n\r\n") == std::string::npos)
	{
		_currentState = CLIENT_PROCESSING_REQUEST;
		_request.parse(_readBuffer, _currentState);
		// TODO: Request routing and server selection
		_currentState = CLIENT_SENDING_RESPONSE;
	}
	else
		_currentState = CLIENT_READING_REQUEST;
}

void Client::sendResponse()
{
	// Check if client is in the correct state
	if (_currentState != CLIENT_SENDING_RESPONSE)
	{
		std::stringstream ss;
		ss << "error sending response client expected state: CLIENT_SENDING_RESPONSE"
		   << " current state: " << _currentState;
		Logger::log(Logger::ERROR, ss.str());
		throw std::runtime_error(ss.str());
	}
	_response.generateResponse(_request);
	_socketFd.writeFile(_response.getResponse());
	_currentState = CLIENT_WAITING_FOR_REQUEST;
}

void Client::setState(State newState)
{
	_currentState = newState;
}

Client::State Client::getCurrentState() const
{
	return _currentState;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

const std::string &Client::getClientIP() const
{
	return _clientAddr.getIP();
}

const Server *Client::getServer() const
{
	return _server;
}

void Client::setServer(const Server *server)
{
	_server = server;
}

/* ************************************************************************** */
