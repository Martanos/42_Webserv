<<<<<<< HEAD
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
=======
#include "../../includes/HTTP/HttpURI.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/Constants.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpURI::HttpURI()
{
}

HttpURI::HttpURI(const HttpURI &other)
{
	*this = other;
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

HttpURI &HttpURI::operator=(const HttpURI &other)
{
	if (this != &other)
	{
		_uriState = other._uriState;
		_method = other._method;
		_URI = other._URI;
		_version = other._version;
		_queryParameters = other._queryParameters;
		_uriSize = other._uriSize;
	}
	return *this;
}
/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::parseBuffer(std::vector<char> &buffer, HttpResponse &response)
{
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
	if (it == buffer.end())
	{
		// If it can't be found check that the buffer has not currently exceeded the size limit of a header
		if (buffer.size() > HTTP::MAX_URI_LINE_SIZE)
		{
			response.setStatus(413, "Request URI Too Large");
			Logger::log(Logger::ERROR, "URI size limit exceeded");
			_uriState = URI_PARSING_ERROR;
		}
		else
			_uriState = URI_PARSING;
		return;
	}

	// Extract request line up to the CLRF
	std::string requestLine(buffer.begin(), it);
	if (requestLine.size() + 2 > HTTP::MAX_URI_LINE_SIZE)
	{
		response.setStatus(413, "Request URI Too Large");
		Logger::log(Logger::ERROR, "URI size limit exceeded");
		_uriState = URI_PARSING_ERROR;
		return;
	}
	_uriSize = requestLine.size() + 2;
	// Clear buffer up to the CLRF
	buffer.erase(buffer.begin(), it + 2);

	// Parse request line
	std::istringstream stream(requestLine);

	if (!(stream >> _method >> _URI >> _version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// Validate URI
	if (_URI.empty() || _URI[0] != '/')
	{
		Logger::log(Logger::ERROR, "Invalid URI: " + _URI);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// Validate version
	if (_version != "HTTP/1.1")
	{
		Logger::log(Logger::ERROR, "Unsupported HTTP version: " + _version);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(505, "HTTP Version Not Supported");
		return;
	}

	_uriState = URI_PARSING_COMPLETE;
}

void HttpURI::sanitizeURI(const Server *server, const Location *location)
{
	std::string path;
	std::string queryParameters;

	// 1. Seperate the URI into the path and the query parameters
	size_t queryPos = _URI.find('?');
	if (queryPos != std::string::npos)
	{
		path = _URI.substr(0, queryPos);
		queryParameters = _URI.substr(queryPos + 1);
	}
	else
	{
		path = _URI;
	}

	// 2. Seperate query parameters into tokens
	std::string token;
	std::istringstream stream(queryParameters);
	while (getline(stream, token, '&'))
	{
		// Seperate into key and value
		size_t keyPos = token.find('=');
		std::string key = token.substr(0, keyPos);
		std::string value = token.substr(keyPos + 1);

		// Decode key and value
		key = StrUtils::percentDecode(key);
		value = StrUtils::percentDecode(value);

		// Add to query parameters
		_queryParameters[key].push_back(value);
	}

	// 3. Sanitize path
	path = StrUtils::percentDecode(path);

	// 4. Combine path and root path
	std::string root;
	if (location && !location->getRoot().empty())
		root = location->getRoot();
	else
		root = server->getRootPath();

	std::string fullPath = root;
	if (!root.empty() && root[root.size() - 1] != '/')
		fullPath += "/";
	fullPath += path;

	char resolvedPath[PATH_MAX];
	if (realpath(fullPath.c_str(), resolvedPath) == NULL)
	{
		Logger::log(Logger::ERROR, "Cannot resolve path: " + fullPath);
		_uriState = URI_PARSING_ERROR;
		return;
	}

	if (std::string(resolvedPath).compare(0, root.size(), root) != 0)
	{
		Logger::log(Logger::ERROR, "Resolved path escapes root: " + std::string(resolvedPath));
		_uriState = URI_PARSING_ERROR;
		return;
	}

	_URI = resolvedPath;
	return;
}

/*
** --------------------------------- ACCESSORS ----------------------------------
*/

const std::string &HttpURI::getURI() const
{
	return _URI;
}

const std::string &HttpURI::getVersion() const
{
	return _version;
}

HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

size_t HttpURI::getURIsize() const
{
	return _uriSize;
}

const std::string &HttpURI::getMethod() const
{
	return _method;
}

const std::map<std::string, std::vector<std::string> > &HttpURI::getQueryParameters() const
{
	return _queryParameters;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_method = "";
	_URI = "";
	_version = "";
	_queryParameters.clear();
>>>>>>> ConfigParserRefactor
}