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
		_sendingState = rhs._sendingState;
		_responseType = rhs._responseType;
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

// Used for responses with no custom body
void HttpResponse::setResponseDefaultBody(int statusCode, const std::string &statusMessage, const Server *server,
										  const Location *location, ResponseType responseType)
{
	_statusCode = statusCode;
	_statusMessage = statusMessage;
	_responseType = responseType;
	_body = DefaultStatusMap::getStatusBody(_statusCode);
	_streamBody = false;
	setHeader(Header("content-type: " + HTTP_RESPONSE_DEFAULT::CONTENT_TYPE));
	setHeader(Header("content-length: " + StrUtils::toString(_body.length())));
	if (location && location->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = location->getRoot() + location->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
	}
	else if (server && server->hasStatusPage(_statusCode))
	{
		std::string statusPagePath = server->getRootPath() + server->getStatusPages().find(_statusCode)->second;
		statusPagePath = FileUtils::normalizePath(statusPagePath);
		if (FileUtils::isFileReadable(statusPagePath))
		{
			_bodyFileDescriptor = FileDescriptor::createFromOpen(statusPagePath.c_str(), O_RDONLY);
			_streamBody = true;
		}
	}
}
// Used when custom body is in memory
void HttpResponse::setResponseCustomBody(int statusCode, const std::string &statusMessage, const std::string &body,
										 const std::string &contentType, ResponseType responseType)
{
	_statusCode = statusCode;
	_statusMessage = statusMessage;
	_responseType = responseType;
	_body = body;
	_streamBody = false;
	setHeader(Header("content-type: " + contentType));
	setHeader(Header("content-length: " + StrUtils::toString(body.length())));
}

// Used when custom body is a file path
// Make sure path is absolute root + filePath sanitized and has undergone validation
void HttpResponse::setResponseFile(int statusCode, const std::string &statusMessage, const std::string &filePath,
								   const std::string &contentType, ResponseType responseType)
{
	_statusCode = statusCode;
	_responseType = responseType;
	_statusMessage = statusMessage;
	Logger::debug("HttpResponse: Setting response file: " + filePath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	_bodyFileDescriptor = FileDescriptor::createFromOpen(filePath.c_str(), O_RDONLY);
	_streamBody = true;
	setHeader(Header("content-type: " + contentType));
	setHeader(Header("content-length: " + StrUtils::toString(_bodyFileDescriptor.getFileSize())));
}

// Used when a redirect is needed
void HttpResponse::setRedirectResponse(const std::string &redirectPath, ResponseType responseType)
{
	_responseType = responseType;
	setStatus(301, "Moved Permanently");
	setHeader(Header("location: " + redirectPath));
	setResponseFile(301, "Moved Permanently", redirectPath, "text/html", responseType);
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
	_sendingState = RESPONSE_FORMATTING_MESSAGE;
	_responseType = SUCCESS;
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

HttpResponse::SendingState HttpResponse::getSendingState() const
{
	return _sendingState;
}

HttpResponse::ResponseType HttpResponse::getResponseType() const
{
	return _responseType;
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

void HttpResponse::setStatus(int code, const std::string &message)
{
	setStatusCode(code);
	setStatusMessage(message);
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
	while (_sendingState != RESPONSE_SENDING_COMPLETE && _sendingState != RESPONSE_SENDING_ERROR &&
		   totalBytesSent < HTTP::DEFAULT_SEND_SIZE)
	{
		switch (_sendingState)
		{
		case RESPONSE_FORMATTING_MESSAGE:
		{
			// Translate response data into a http string format assume content type and length are set if needed
			_rawResponse = _version + " " + StrUtils::toString(_statusCode) + " " + _statusMessage + "\r\n";
			_version.clear();
			_statusMessage.clear();
			for (std::vector<Header>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
			{
				_rawResponse += it->getDirective() + ": " + it->getValues()[0] + "\r\n";
			}
			_rawResponse += "\r\n";
			_headers.clear();
			// Append body if its already in memory
			if (!_streamBody)
			{
				_rawResponse += _body;
				_body.clear();
			}
			_sendingState = RESPONSE_SENDING_MESSAGE;
			break;
		}
		case RESPONSE_SENDING_MESSAGE:
		{
			ssize_t sendBufferSize = HTTP::DEFAULT_SEND_SIZE - totalBytesSent;
			if (sendBufferSize == 0)
				return;
			std::string bytesToSend = _rawResponse.substr(0, sendBufferSize);
			ssize_t bytesSent = send(clientFd.getFd(), bytesToSend.c_str(), bytesToSend.length(), 0);
			if (bytesSent > 0)
			{
				totalBytesSent += bytesSent;
				_rawResponse = _rawResponse.substr(bytesSent);
				if (_rawResponse.empty() && !_streamBody)
					_sendingState = RESPONSE_SENDING_COMPLETE;
				else if (_rawResponse.empty() && _streamBody) // Case body is being streamed, still more to send
					_sendingState = RESPONSE_SENDING_BODY;
			}
			else
				_sendingState = RESPONSE_SENDING_ERROR;
			break;
		}
		case RESPONSE_SENDING_BODY:
		{
			// SafeGuard should never occur
			if (!_bodyFileDescriptor.isOpen())
			{
				_sendingState = RESPONSE_SENDING_ERROR;
				return;
			}
			std::string buffer;
			ssize_t sendBufferSize = HTTP::DEFAULT_SEND_SIZE - totalBytesSent;
			if (sendBufferSize == 0)
				return;
			buffer.resize(sendBufferSize); // Read the remaining bytes needed to send
			ssize_t bytesRead = _bodyFileDescriptor.readFile(buffer);
			if (bytesRead < 0)
				_sendingState = RESPONSE_SENDING_ERROR;
			else if (bytesRead == 0)
				_sendingState = RESPONSE_SENDING_COMPLETE;
			else if (bytesRead > 0)
			{
				ssize_t bytesSent = send(clientFd.getFd(), buffer.c_str(), buffer.length(), 0);
				if (bytesSent > 0)
					totalBytesSent += bytesSent;
				else
					_sendingState = RESPONSE_SENDING_ERROR;
			}
			break;
		}
		default:
			return;
		}
	}
}

/* ************************************************************************** */
