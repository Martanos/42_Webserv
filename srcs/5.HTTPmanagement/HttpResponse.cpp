#include "../../includes/HTTP/HttpResponse.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/DefaultStatusMap.hpp"
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
		_bytesSent = rhs._bytesSent;
		_rawResponse = rhs._rawResponse;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

bool HttpResponse::isEmpty() const
{
	return _body.empty();
}

void HttpResponse::setStatus(int code, const std::string &message)
{
	_statusCode = code;
	_statusMessage = message;
}

void HttpResponse::setHeader(const std::string &name, const std::string &value)
{
	_headers[name] = value;
}

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
	Logger::debug("HttpResponse: Set body with " + StrUtils::toString(body.length()) + " bytes");
}

void HttpResponse::setBody(const Location *location, const Server *server)
{
	if (location && location->hasStatusPage(_statusCode))
	{
		_body = location->getStatusPages().find(_statusCode)->second;
	}
	else if (server && server->hasStatusPage(_statusCode))
	{
		_body = server->getStatusPages().find(_statusCode)->second;
	}
	else if (DefaultStatusMap::hasStatusBody(_statusCode))
	{
		_body = DefaultStatusMap::getStatusBody(_statusCode);
	}
	else
	{
		Logger::log(Logger::ERROR, "No status page found for status code: " + StrUtils::toString(_statusCode));
		_body = DefaultStatusMap::getStatusBody(500);
	}
	setHeader("Content-Type", "text/html");
	setHeader("Content-Length", StrUtils::toString(_body.length()));
}

// Formats the response into a HTTP 1.1 compliant format
std::string HttpResponse::toString() const
{
	std::stringstream response;

	// Status line
	response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << "\r\n";
	}

	// Empty line between headers and body
	response << "\r\n";

	// Body (if any)
	if (!_body.empty())
	{
		response << _body;
	}

	return response.str();
}

void HttpResponse::reset()
{
	_statusCode = 0;
	_statusMessage = "";
	_version = "";
	_headers.clear();
	_body = "";
	_bytesSent = 0;
	_rawResponse = "";
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
	return _version;
}

std::map<std::string, std::string> HttpResponse::getHeaders() const
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

void HttpResponse::setHeaders(const std::map<std::string, std::string> &headers)
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

void HttpResponse::setAutoFields()
{
	// Set default version if not set
	if (_version.empty())
	{
		_version = "HTTP/1.1";
	}

	// Set default status message if not set
	if (_statusMessage.empty())
	{
		switch (_statusCode)
		{
		case 200:
			_statusMessage = "OK";
			break;
		case 201:
			_statusMessage = "Created";
			break;
		case 204:
			_statusMessage = "No Content";
			break;
		case 301:
			_statusMessage = "Moved Permanently";
			break;
		case 302:
			_statusMessage = "Found";
			break;
		case 400:
			_statusMessage = "Bad Request";
			break;
		case 401:
			_statusMessage = "Unauthorized";
			break;
		case 403:
			_statusMessage = "Forbidden";
			break;
		case 404:
			_statusMessage = "Not Found";
			break;
		case 405:
			_statusMessage = "Method Not Allowed";
			break;
		case 413:
			_statusMessage = "Payload Too Large";
			break;
		case 414:
			_statusMessage = "URI Too Long";
			break;
		case 500:
			_statusMessage = "Internal Server Error";
			break;
		case 501:
			_statusMessage = "Not Implemented";
			break;
		case 503:
			_statusMessage = "Service Unavailable";
			break;
		default:
			_statusMessage = "Unknown";
			break;
		}
	}

	// Set Content-Length if body exists and not already set
	if (!_body.empty() && _headers.find("content-length") == _headers.end())
	{
		_headers["content-length"] = StrUtils::toString(_body.length());
	}

	// Set Date header if not already set
	if (_headers.find("date") == _headers.end())
	{
		std::time_t now = std::time(0);
		char buf[80];
		std::strftime(buf, sizeof(buf), "%a, %d %b %Y %H:%M:%S GMT", std::gmtime(&now));
		_headers["date"] = std::string(buf);
	}

	// Set Server header if not already set
	if (_headers.find("server") == _headers.end())
	{
		_headers["server"] = "42_Webserv/1.0";
	}
}

/* ************************************************************************** */
