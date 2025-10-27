#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/DefaultStatusMap.hpp"
#include "../../includes/Global/FileUtils.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
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
		_bytesSent = rhs._bytesSent;
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
}

// Used when location and server have not been identified
void HttpResponse::setResponse(int statusCode, const std::string &statusMessage)
{
	setStatus(statusCode, statusMessage);
	_body = DefaultStatusMap::getStatusBody(statusCode);
}

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
	_bytesSent = 0;
	_rawResponse = "";
	_httpResponseState = RESPONSE_SENDING_URI;
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

size_t HttpResponse::getBytesSent() const
{
	return _bytesSent;
}

std::string HttpResponse::getRawResponse() const
{
	return _rawResponse;
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

void HttpResponse::setBytesSent(size_t bytesSent)
{
	_bytesSent = bytesSent;
}

void HttpResponse::setRawResponse(const std::string &rawResponse)
{
	_rawResponse = rawResponse;
}

/* ************************************************************************** */
