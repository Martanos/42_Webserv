#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/DefaultStatusMap.hpp"
#include "../../includes/Global/FileUtils.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HTTP.hpp"
/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpResponse::HttpResponse()
{
	reset();
}

HttpResponse::HttpResponse(const HttpResponse &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpResponse::~HttpResponse()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpResponse &HttpResponse::operator=(HttpResponse const &rhs)
{
	if (this != &rhs)
	{
		_statusCode = rhs._statusCode;
		_statusMessage = rhs._statusMessage;
		_version = rhs._version;
		_headers = rhs._headers;
		_body = rhs._body;
		_streamBody = rhs._streamBody;
		_bodyFileDescriptor = rhs._bodyFileDescriptor;
		_rawResponse = rhs._rawResponse;
		_httpResponseState = rhs._httpResponseState;
	}
	return *this;
}

/*
** --------------------------------- PRIVATE METHODS ----------------------------------
*/

void HttpResponse::_getDateHeader()
{
	std::time_t now = std::time(0);
	char buf[80];
	std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
	insertHeader(Header("date: " + std::string(buf) + " GMT"));
}

void HttpResponse::_setServerHeader()
{
	insertHeader(Header("server: " + HTTP_RESPONSE_DEFAULT::SERVER));
}

void HttpResponse::_setContentLengthHeader()
{
	if (!_body.empty())
	{
		insertHeader(Header("content-length: " + StrUtils::toString(_body.length())));
	}
}

void HttpResponse::_setVersionHeader()
{
	_version = HTTP_RESPONSE_DEFAULT::VERSION;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpResponse::setStatus(int code, const std::string &message)
{
	_statusCode = code;
	_statusMessage = message;
}

// Replaces a header if it exists else inserts it
void HttpResponse::setHeader(const Header &header)
{
	for (std::vector<Header>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (*it == header)
		{
			*it = header;
			return;
		}
	}
	_headers.push_back(header);
}
// Inserts/mergers a header if it exists
void HttpResponse::insertHeader(const Header &header)
{
	for (std::vector<Header>::iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (*it == header)
		{
			it->merge(header);
			return;
		}
	}
	_headers.push_back(header);
}

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
	_streamBody = false;
	setHeader(Header("content-type: text/html"));
	setHeader(Header("content-length: " + StrUtils::toString(body.length())));
}

// TODO: consolidate setting response into multiple functions types of responses:
// 1. setSuccessResponse
// 2. setErrorResponse
// 3. setRedirectResponse
// 4. setFileResponse
// 5. setCGIResponse

// Used when location and server have not been identified
void HttpResponse::setResponse(int statusCode, const std::string &statusMessage)
{
	setStatus(statusCode, statusMessage);
	_body = DefaultStatusMap::getStatusBody(statusCode);
}

// Used when access to body is not known yet
void HttpResponse::setResponse(int statusCode, const std::string &statusMessage, const Server *server,
							   const Location *location, const std::string &filePath)
{
	setStatus(statusCode, statusMessage);
	if (location && location->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = filePath + location->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
		else
			_body = DefaultStatusMap::getStatusBody(_statusCode);
	}
	else if (server && server->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = filePath + server->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
		else
			_body = DefaultStatusMap::getStatusBody(_statusCode);
	}
	else
		_body = DefaultStatusMap::getStatusBody(_statusCode);
}

// Used when the body is already known
void HttpResponse::setResponse(int statusCode, const std::string &statusMessage, const std::string &body,
							   const std::string &contentType)
{
	setStatus(statusCode, statusMessage);
	_body = body;
	_streamBody = false;
	setHeader(Header("content-type: " + contentType));
	setHeader(Header("content-length: " + StrUtils::toString(body.length())));
}

// Used when a redirect is needed
void HttpResponse::setRedirectResponse(const std::string &redirectPath)
{
	setStatus(301, "Moved Permanently");
	setHeader(Header("location: " + redirectPath));
	setResponse(301, "Moved Permanently", "", "text/html");
}

// Formats the response into a HTTP 1.1 compliant format
std::string HttpResponse::toString() const
{
	std::stringstream response;

	// Status line
	response << _version << " " << _statusCode << " " << _statusMessage << "\r\n";

	// Headers
	for (std::vector<Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << *it << "\r\n";
	}

	// Empty line between headers and body
	response << "\r\n";

	// Body (if any)
	if (_streamBody)
	{
		response << _bodyFileDescriptor.readFile();
	}
	else
	{
		response << _body;
	}

	return response.str();
}

void HttpResponse::reset()
{
	_statusCode = HTTP_RESPONSE_DEFAULT::DEFAULT_STATUS_CODE;
	_statusMessage = HTTP_RESPONSE_DEFAULT::DEFAULT_STATUS_MESSAGE;
	_headers.clear();
	_body = "";
	_streamBody = false;
	_bodyFileDescriptor = FileDescriptor();
	_httpResponseState = RESPONSE_FORMATTING_MESSAGE;
	_getDateHeader();
	_setServerHeader();
	_setVersionHeader();
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

int HttpResponse::getStatusCode() const
{
	return _statusCode;
}

std::string HttpResponse::getStatusMessage() const
{
	return _statusMessage;
}

std::string HttpResponse::getVersion() const
{
	for (std::vector<Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		if (it->getDirective() == "version")
		{
			return it->getValues()[0];
		}
	}
	return HTTP_RESPONSE_DEFAULT::VERSION;
}

std::vector<Header> HttpResponse::getHeaders() const
{
	return _headers;
}

std::string HttpResponse::getBody() const
{
	return _body;
}

std::string HttpResponse::getRawResponse() const
{
	return _rawResponse;
}

HttpResponse::HttpResponseState HttpResponse::getState() const
{
	return _httpResponseState;
}
/*
** --------------------------------- Mutator ---------------------------------
*/

void HttpResponse::setStatusCode(int code)
{
	_statusCode = code;
}

void HttpResponse::setStatusMessage(const std::string &message)
{
	_statusMessage = message;
}

void HttpResponse::setVersion(const std::string &version)
{
	_version = version;
}

void HttpResponse::setHeaders(const std::vector<Header> &headers)
{
	_headers = headers;
}

void HttpResponse::setRawResponse(const std::string &rawResponse)
{
	_rawResponse = rawResponse;
}

void HttpResponse::setLastModifiedHeader()
{
	std::time_t lastModified = std::time(0);
	char buffer[100];
	struct tm *tm = std::gmtime(&lastModified);
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", tm);
	setHeader(Header("last-modified: " + std::string(buffer) + " GMT"));
}

void HttpResponse::setBody(const Location *location, const Server *server)
{
	// Attempt to set body based on current response code and whether the location or server has a status page

	// Location takes precedence over server
	if (location && location->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = location->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
		else
			_body = DefaultStatusMap::getStatusBody(_statusCode);
	}
	else if (server && server->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = server->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
		else
			_body = DefaultStatusMap::getStatusBody(_statusCode);
	}
	else
		_body = DefaultStatusMap::getStatusBody(_statusCode);
}

void HttpResponse::sendResponse(const FileDescriptor &clientFd, ssize_t &totalBytesSent)
{
	totalBytesSent = 0;
	while (_httpResponseState != RESPONSE_SENDING_COMPLETE && _httpResponseState != RESPONSE_SENDING_ERROR)
	{
		switch (_httpResponseState)
		{
		case RESPONSE_FORMATTING_MESSAGE:
		{
			// Translate response data into a http string format assume content type and length are set if needed
			_rawResponse = _version + " " + StrUtils::toString(_statusCode) + " " + _statusMessage + "\r\n";
			for (std::vector<Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
			{
				_rawResponse += it->getDirective() + ": " + it->getValues()[0] + "\r\n";
			}
			_rawResponse += "\r\n";
			// Append body if its already in memory
			if (!_streamBody)
				_rawResponse += _body;
			_httpResponseState = RESPONSE_SENDING_MESSAGE;
			break;
		}
		case RESPONSE_SENDING_MESSAGE:
		{
			std::string bytesToSend = _rawResponse.substr(0, HTTP::DEFAULT_SEND_SIZE);
			ssize_t bytesSent = send(clientFd.getFd(), bytesToSend.c_str(), bytesToSend.length(), 0);
			if (bytesSent > 0)
			{
				totalBytesSent += bytesSent;
				_rawResponse = _rawResponse.substr(bytesSent);
			}
			else
			{
				_httpResponseState = RESPONSE_SENDING_ERROR;
				Logger::error("HttpResponse: Failed to send message for client: " +
							  StrUtils::toString(clientFd.getFd()) + ": " + strerror(errno));
				return;
			}

			// Determine next steps
			if (totalBytesSent == HTTP::DEFAULT_SEND_SIZE && !_rawResponse.empty())
				return; // Case sent 4096 bytes, still more to send
			else if (totalBytesSent == HTTP::DEFAULT_SEND_SIZE && _rawResponse.empty() && !_streamBody)
			{
				// Case sent 4096 bytes, no more to send, no body to send
				_httpResponseState = RESPONSE_SENDING_COMPLETE;
				break;
			}
			else if (_streamBody && _bodyFileDescriptor.isOpen()) // Case body is being streamed, still more to send
			{
				_httpResponseState = RESPONSE_SENDING_BODY;
				break;
			}
			break;
		}
		case RESPONSE_SENDING_BODY:
		{
			if (totalBytesSent == HTTP::DEFAULT_SEND_SIZE)
				return; // Case sent 4096 bytes, still more to send
			std::string buffer;
			buffer.resize(HTTP::DEFAULT_SEND_SIZE - totalBytesSent); // Read the remaining bytes needed to send
			ssize_t bytesRead = _bodyFileDescriptor.readFile(buffer);
			if (bytesRead < 0)
			{
				_httpResponseState = RESPONSE_SENDING_ERROR;
				Logger::error("HttpResponse: Failed to read body for client: " + StrUtils::toString(clientFd.getFd()) +
							  ": " + strerror(errno));
				return;
			}
			else if (bytesRead == 0)
			{
				_httpResponseState = RESPONSE_SENDING_COMPLETE;
				return;
			}
			ssize_t bytesSent = send(clientFd.getFd(), buffer.c_str(), buffer.length(), 0);
			if (bytesSent > 0)
			{
				_httpResponseState = RESPONSE_SENDING_MESSAGE;
			}
			else if (bytesSent == 0)
				_httpResponseState = RESPONSE_SENDING_COMPLETE;
			else
			{
				_httpResponseState = RESPONSE_SENDING_ERROR;
				Logger::error("HttpResponse: Failed to send body for client: " + StrUtils::toString(clientFd.getFd()) +
							  ": " + strerror(errno));
				return;
			}
			break;
		}
		default:
			break;
		}
	}
}

/* ************************************************************************** */
