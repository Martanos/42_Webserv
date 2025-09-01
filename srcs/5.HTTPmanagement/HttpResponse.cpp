#include "HttpResponse.hpp"

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
}

std::string HttpResponse::toString() const
{
	std::stringstream response;

	// Status line
	response << "HTTP/1.1 " << _statusCode << " " << _statusMessage << "\r\n";

	// Headers
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin();
		 it != _headers.end(); ++it)
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

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
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
