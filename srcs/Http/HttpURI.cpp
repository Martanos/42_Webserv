#include "../../includes/Http/HttpURI.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpURI::HttpURI()
{
	_uriState = URI_PARSING;
	_uriSize = 0;
	_method.clear();
	_URI.clear();
	_rawURI.clear();
	_version.clear();
	_queryParameters.clear();
	_queryString.clear();
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
		_rawURI = other._rawURI;
		_version = other._version;
		_queryParameters = other._queryParameters;
		_queryString = other._queryString;
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
		if (buffer.size() > HTTP::DEFAULT_CLIENT_MAX_REQUEST_LINE_SIZE)
		{
			response.setResponseDefaultBody(413, "Request URI Too Large", NULL, HttpResponse::FATAL_ERROR);
			Logger::debug("URI size limit exceeded", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_uriState = URI_PARSING_ERROR;
		}
		else
			_uriState = URI_PARSING;
		return;
	}

	// Extract request line up to the CLRF
	std::string requestLine(buffer.begin(), it);
	if (requestLine.size() + 2 > HTTP::DEFAULT_CLIENT_MAX_REQUEST_LINE_SIZE)
	{
		response.setResponseDefaultBody(413, "Request URI Too Large", NULL, HttpResponse::FATAL_ERROR);
		Logger::debug("URI size limit exceeded", __FILE__, __LINE__, __PRETTY_FUNCTION__);
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
		Logger::debug("Invalid request line: " + requestLine, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(400, "Invalid request line: " + requestLine, NULL, HttpResponse::FATAL_ERROR);
		return;
	}

	_rawURI = _URI;

	// Validate URI
	if (_URI.empty() || _URI[0] != '/')
	{
		Logger::debug("Invalid URI: " + _URI, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(400, "Invalid URI: " + _URI, NULL, HttpResponse::FATAL_ERROR);
		return;
	}

	// Validate version
	if (_version != "HTTP/1.1")
	{
		Logger::debug("Unsupported HTTP version: " + _version, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(505, "HTTP Version Not Supported: " + _version, NULL,
										HttpResponse::FATAL_ERROR);
		return;
	}

	_uriState = URI_PARSING_COMPLETE;
}

void HttpURI::sanitizeURI(const Location *location, HttpResponse &response)
{
	std::string path;
	// 1. Seperate the URI into the path and the query parameters
	size_t queryPos = _URI.find('?');
	if (queryPos != std::string::npos)
	{
		path = _URI.substr(0, queryPos);
		_queryString = _URI.substr(queryPos + 1);
	}
	else
		path = _URI;

	// 2. Seperate query parameters into tokens
	std::string token;
	std::istringstream stream(_queryString);
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

	// Decode the uri path
	path = StrUtils::percentDecode(path);

	// Get the root path from location
	const std::string *root = location->getRootPath();
	if (root == NULL || root->empty())
	{
		_uriState = URI_PARSING_ERROR;
		Logger::debug("Location root path is not set", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(500, "Location root path is not set", location, HttpResponse::FATAL_ERROR);
		return;
	}

	// Combine root and path
	std::string fullPath = *root;
	if (root->at(root->size() - 1) != '/' && path[0] != '/')
		fullPath += "/";
	fullPath += path;

	// Use realpath to attempt to resolve the path
	char resolvedPath[PATH_MAX];

	// First attempt: resolve full path
	if (realpath(fullPath.c_str(), resolvedPath) != NULL)
	{
		std::string resolved(resolvedPath);

		// containment check
		if (resolved.compare(0, location->getRootPath()->size(), *location->getRootPath()) != 0)
		{
			_uriState = URI_PARSING_ERROR;
			response.setResponseDefaultBody(403, "Forbidden", location, HttpResponse::ERROR);
			return;
		}

		_sanitizedURI = resolved;
		return;
	}

	// Branch based on location type
	switch (location->getLocationType())
	{
	case Location::CGI:
	case Location::UPLOAD:
	{
		// fallback: resolve directory
		size_t lastSlash = fullPath.find_last_of('/');
		std::string directoryPath = (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : fullPath;

		if (directoryPath.empty())
			directoryPath = *location->getRootPath();

		if (realpath(directoryPath.c_str(), resolvedPath) == NULL)
		{
			_uriState = URI_PARSING_ERROR;
			response.setResponseDefaultBody(404, "Not Found", location, HttpResponse::ERROR);
			return;
		}

		std::string resolvedDir(resolvedPath);

		// containment check
		if (resolvedDir.compare(0, location->getRootPath()->size(), *location->getRootPath()) != 0)
		{
			_uriState = URI_PARSING_ERROR;
			response.setResponseDefaultBody(403, "Forbidden", location, HttpResponse::ERROR);
			return;
		}

		// reattach remainder (script name or upload filename)
		std::string remainder = (lastSlash == std::string::npos) ? std::string() : fullPath.substr(lastSlash + 1);

		if (!remainder.empty())
		{
			if (!resolvedDir.empty() && resolvedDir[resolvedDir.size() - 1] != '/')
				resolvedDir += "/";
			resolvedDir += remainder;
		}

		_sanitizedURI = resolvedDir;
		return;
	}

	case Location::STATIC:
	case Location::REDIRECT:
	default:
		// For these types, no fallback — fail immediately
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(404, "Not Found", location, HttpResponse::ERROR);
		return;
	}
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

const std::string &HttpURI::getQueryString() const
{
	return _queryString;
}

const std::string &HttpURI::getSanitizedURI() const
{
	return _sanitizedURI;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_method.clear();
	_URI.clear();
	_rawURI.clear();
	_version.clear();
	_queryParameters.clear();
	_queryString.clear();
	_sanitizedURI.clear();
	_uriSize = 0;
}

const std::string &HttpURI::getRawURI() const
{
	return _rawURI;
}
