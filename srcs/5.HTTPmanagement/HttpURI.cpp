#include "HttpURI.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpURI::HttpURI()
{
	_uriState = URI_PARSING;
	_rawURI = "";
	_method = "";
	_uri = "";
	_version = "";
}

HttpURI::HttpURI(const HttpURI &src)
{
	_uriState = src._uriState;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpURI::~HttpURI()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpURI &HttpURI::operator=(HttpURI const &rhs)
{
	if (this != &rhs)
	{
		_uriState = rhs._uriState;
		_rawURI = rhs._rawURI;
		_method = rhs._method;
		_uri = rhs._uri;
		_version = rhs._version;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpURI::parseBuffer(std::string &buffer, HttpResponse &response)
{
	if (buffer.empty())
	{
		return URI_PARSING;
	}
	size_t newlinePos = buffer.find("\r\n");
	if (newlinePos == std::string::npos)
	{
		return URI_PARSING;
	}
	std::string requestLine = buffer.substr(0, newlinePos);
	if (requestLine.size() > sysconf(_SC_PAGE_SIZE))
	{
		Logger::log(Logger::ERROR, "Request line too long");
		response.setStatusCode(HTTP::STATUS_URI_TOO_LONG);
		response.setStatusMessage("Request line too long");
		response.setBody("Request line too long");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	buffer.erase(0, newlinePos + 2);
	std::istringstream iss(requestLine);
	std::string method, uri, version;
	std::string extraInfo;
	if (!(iss >> method >> uri >> version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	else if (method.empty() || uri.empty() || version.empty())
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	else if (version != HTTP::HTTP_VERSION)
	{
		Logger::log(Logger::ERROR, "Invalid HTTP version: " + version);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	else if (!iss.eof())
	{
		iss >> extraInfo;
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine + " - Extra info: " + extraInfo);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	else if (std::find(HTTP::SUPPORTED_METHODS, HTTP::SUPPORTED_METHODS + sizeof(HTTP::SUPPORTED_METHODS), method) == HTTP::SUPPORTED_METHODS + sizeof(HTTP::SUPPORTED_METHODS))
	{
		Logger::log(Logger::ERROR, "Invalid HTTP method: " + method);
		response.setStatusCode(HTTP::STATUS_BAD_REQUEST);
		response.setStatusMessage("Bad Request");
		response.setBody("Bad Request");
		response.setHeader("Content-Length", StringUtils::toString(response.getBody().size()));
		response.setHeader("Connection", "close");
		return URI_PARSING_ERROR;
	}
	else
	{
		_method = method;
		_uri = uri;
		_version = version;
	}
	_rawURI = requestLine;
	_uriState = URI_PARSING_COMPLETE;
	return URI_PARSING_COMPLETE;
}

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_rawURI = "";
	_method = "";
	_uri = "";
	_version = "";
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

std::string HttpURI::getRawURI() const
{
	return _rawURI;
}

std::string HttpURI::getMethod() const
{
	return _method;
}

std::string HttpURI::getURI() const
{
	return _uri;
}

std::string HttpURI::getVersion() const
{
	return _version;
}

void HttpURI::setURIState(URIState uriState)
{
	_uriState = uriState;
}

void HttpURI::setRawURI(const std::string &rawURI)
{
	_rawURI = rawURI;
}

void HttpURI::setMethod(const std::string &method)
{
	_method = method;
}

void HttpURI::setURI(const std::string &uri)
{
	_uri = uri;
}

void HttpURI::setVersion(const std::string &version)
{
	_version = version;
}

/* ************************************************************************** */
