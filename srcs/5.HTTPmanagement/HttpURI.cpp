#include "../../includes/HttpURI.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/StringUtils.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpURI::parseBuffer(RingBuffer &buffer, HttpResponse &response)
{
	if (_uriState == URI_PARSING_COMPLETE)
		return URI_PARSING_COMPLETE;

	if (_uriState == URI_PARSING_ERROR)
		return URI_PARSING_ERROR;

	// Transfer data from buffer to raw URI
	size_t transferred = _rawURI.transferFrom(buffer, buffer.readable());
	if (transferred == 0)
		return URI_PARSING;

	// Look for end of request line (CRLF or LF)
	std::string rawData;
	_rawURI.peekBuffer(rawData, _rawURI.readable());

	size_t lineEnd = rawData.find("\r\n");
	if (lineEnd == std::string::npos)
	{
		lineEnd = rawData.find('\n');
		if (lineEnd == std::string::npos)
		{
			// Still need more data
			return URI_PARSING;
		}
		lineEnd += 1; // Include the \n
	}
	else
	{
		lineEnd += 2; // Include the \r\n
	}

	// Extract request line
	std::string requestLine;
	_rawURI.readBuffer(requestLine, lineEnd);

	// Parse request line
	std::istringstream stream(requestLine);
	std::string method, uri, version;

	if (!(stream >> method >> uri >> version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return URI_PARSING_ERROR;
	}

	// Validate method
	std::vector<std::string> validMethods;
	validMethods.push_back("GET");
	validMethods.push_back("POST");
	validMethods.push_back("DELETE");

	if (std::find(validMethods.begin(), validMethods.end(), method) == validMethods.end())
	{
		Logger::log(Logger::ERROR, "Unsupported HTTP method: " + method);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(501, "Not Implemented");
		return URI_PARSING_ERROR;
	}

	// Validate URI
	if (uri.empty() || uri[0] != '/')
	{
		Logger::log(Logger::ERROR, "Invalid URI: " + uri);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return URI_PARSING_ERROR;
	}

	// Validate version
	if (version != "HTTP/1.1" && version != "HTTP/1.0")
	{
		Logger::log(Logger::ERROR, "Unsupported HTTP version: " + version);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(505, "HTTP Version Not Supported");
		return URI_PARSING_ERROR;
	}

	// Store parsed values
	_method = method;
	_uri = uri;
	_version = version;

	Logger::debug("HttpURI: Parsed request line: " + method + " " + uri + " " + version);
	_uriState = URI_PARSING_COMPLETE;
	return URI_PARSING_COMPLETE;
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

RingBuffer &HttpURI::getRawURI()
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

/*
** --------------------------------- MUTATORS --------------------------------
*/

void HttpURI::setURIState(URIState uriState)
{
	_uriState = uriState;
}

void HttpURI::setRawURI(RingBuffer &rawURI)
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

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_rawURI.clear();
	_method = "";
	_uri = "";
	_version = "";
}