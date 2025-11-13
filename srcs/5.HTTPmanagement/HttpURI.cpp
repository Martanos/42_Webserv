#include "../../includes/HTTP/HttpURI.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HTTP.hpp"
#include "../../includes/HTTP/HttpResponse.hpp"
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
			response.setResponseDefaultBody(413, "Request URI Too Large", NULL, NULL, HttpResponse::FATAL_ERROR);
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
		response.setResponseDefaultBody(413, "Request URI Too Large", NULL, NULL, HttpResponse::FATAL_ERROR);
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
		response.setResponseDefaultBody(400, "Invalid request line: " + requestLine, NULL, NULL,
										HttpResponse::FATAL_ERROR);
		return;
	}

	_rawURI = _URI;

	// Validate URI
	if (_URI.empty() || _URI[0] != '/')
	{
		Logger::debug("Invalid URI: " + _URI, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(400, "Invalid URI: " + _URI, NULL, NULL, HttpResponse::FATAL_ERROR);
		return;
	}

	// Validate version
	if (_version != "HTTP/1.1")
	{
		Logger::debug("Unsupported HTTP version: " + _version, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(505, "HTTP Version Not Supported: " + _version, NULL, NULL,
										HttpResponse::FATAL_ERROR);
		return;
	}

	_uriState = URI_PARSING_COMPLETE;
}

void HttpURI::sanitizeURI(const Server *server, const Location *location, HttpResponse &response)
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
	{
		path = _URI;
	}

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

	// 3. Sanitize path
	path = StrUtils::percentDecode(path);

	// 4. Combine path and root path
	std::string root;
	if (location && !location->getRoot().empty())
		root = location->getRoot();
	else
		root = server->getRootPath();

	std::string fullPath = root;
	if (!root.empty() && root[root.size() - 1] != '/' && path[0] != '/')
		fullPath += "/";
	fullPath += path;

	char resolvedPath[PATH_MAX];
	if (realpath(fullPath.c_str(), resolvedPath) == NULL)
	{
		size_t lastSlash = fullPath.find_last_of('/');
		std::string directoryPath;
		if (lastSlash != std::string::npos)
		{
			directoryPath = fullPath.substr(0, lastSlash);
		}
		else
		{
			directoryPath = fullPath;
		}

		if (directoryPath.empty())
			directoryPath = root;

		if (!directoryPath.empty() && realpath(directoryPath.c_str(), resolvedPath) != NULL)
		{
			std::string resolvedDir(resolvedPath);
			if (resolvedDir.compare(0, root.size(), root) != 0)
			{
				_uriState = URI_PARSING_ERROR;
				Logger::debug("Resolved directory escapes root: " + resolvedDir, __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				response.setResponseDefaultBody(403, "Forbidden", NULL, NULL, HttpResponse::FATAL_ERROR);
				return;
			}

			std::string remainder = (lastSlash == std::string::npos) ? std::string() : fullPath.substr(lastSlash + 1);
			if (!remainder.empty())
			{
				if (!resolvedDir.empty() && resolvedDir[resolvedDir.size() - 1] != '/')
					resolvedDir += "/";
				resolvedDir += remainder;
			}
			_URI = resolvedDir;
			return;
		}

		_uriState = URI_PARSING_ERROR;
		Logger::debug("Cannot resolve path: " + fullPath, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(404, "Not Found", NULL, NULL, HttpResponse::FATAL_ERROR);
		return;
	}

	if (std::string(resolvedPath).compare(0, root.size(), root) != 0)
	{
		_uriState = URI_PARSING_ERROR;
		Logger::debug("Resolved path escapes root: " + std::string(resolvedPath), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(403, "Forbidden", NULL, NULL, HttpResponse::FATAL_ERROR);
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

const std::string &HttpURI::getQueryString() const
{
	return _queryString;
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
}

const std::string &HttpURI::getRawURI() const
{
	return _rawURI;
}