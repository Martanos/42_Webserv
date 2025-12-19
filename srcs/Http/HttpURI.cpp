#include "../../includes/Http/HttpURI.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/Utils/FileUtils.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpURI::HttpURI()
{
	reset();
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
		// Parsing
		_uriState = other._uriState;
		_rawURISize = other._rawURISize;

		// Request line
		_method = other._method;
		_rawPath = other._rawPath;
		_version = other._version;

		// URI Versions
		_rawPath = other._rawPath;
		_decodedPath = other._decodedPath;
		_resolvedPath = other._resolvedPath;

		// Query parameters
		_rawQueryString = other._rawQueryString;
		_decodedQueryString = other._decodedQueryString;
		_queryParameters = other._queryParameters;
	}
	return *this;
}

/*
** --------------------------------- VALIDATION HELPERS----------------------------------
*/

bool HttpURI::_validateMethod(const std::string &method, HttpResponse &response) const
{
	if (method.empty())
	{
		response.setResponseDefaultBody(400, "Empty method", NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	else if (!StrUtils::isValidToken(method))
	{
		response.setResponseDefaultBody(400, "Invalid method: " + method, NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	return true;
}

bool HttpURI::_validatePath(const std::string &decodedPath, HttpResponse &response) const
{
	if (decodedPath.empty() || decodedPath[0] != '/')
	{
		response.setResponseDefaultBody(400, "Invalid path: Path is empty or does not start with '/' " + decodedPath,
										NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	for (size_t i = 0; i < decodedPath.size(); ++i)
	{
		unsigned char c = decodedPath[i];
		if (c < 0x20 || c == 0x7F)
		{
			response.setResponseDefaultBody(400, "Invalid path: contains control characters" + decodedPath, NULL,
											HttpResponse::FATAL_ERROR);
			return false; // control chars not allowed
		}
	}
	return true;
}

bool HttpURI::_validateVersion(const std::string &version, HttpResponse &response) const
{
	if (version.empty())
	{
		response.setResponseDefaultBody(400, "Empty version", NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	else if (!StrUtils::isValidToken(version))
	{
		response.setResponseDefaultBody(400, "Invalid version: " + version, NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	else if (version != "HTTP/1.1")
	{
		response.setResponseDefaultBody(505, "HTTP Version Not Supported: " + version, NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	return true;
}

void HttpURI::_parseQueryParameters(const std::string &rawQueryString)
{
	std::string token;
	_decodedQueryString = StrUtils::percentDecode(rawQueryString, true);
	std::istringstream stream(_decodedQueryString);
	while (getline(stream, token, '&'))
	{
		// Seperate into key and value
		size_t keyPos = token.find('=');
		std::string key = token.substr(0, keyPos);
		std::string value = token.substr(keyPos + 1);

		// Add to query parameters
		_queryParameters[key].push_back(value);
	}
}

/*
** --------------------------------- MAIN PARSING METHOD ----------------------------------
*/

void HttpURI::parseBuffer(std::vector<char> &buffer, HttpResponse &response)
{
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), HTTP::CRLF, HTTP::CRLF + 2);
	if (it == buffer.end())
	{
		if (buffer.size() > HTTP::DEFAULT_CLIENT_MAX_REQUEST_LINE_SIZE)
		{
			response.setResponseDefaultBody(413, "Request URI Too Large", NULL, HttpResponse::FATAL_ERROR);
			_uriState = URI_PARSING_ERROR;
		}
		return;
	}

	// Extract request line up to the CLRF
	std::string requestLine(buffer.begin(), it);
	if (requestLine.size() + 2 > HTTP::DEFAULT_CLIENT_MAX_REQUEST_LINE_SIZE)
	{
		response.setResponseDefaultBody(413, "Request URI Too Large", NULL, HttpResponse::FATAL_ERROR);
		_uriState = URI_PARSING_ERROR;
		return;
	}

	_rawURISize = requestLine.size() + 2;
	// Clear buffer up to the CLRF
	buffer.erase(buffer.begin(), it + 2);

	// Parse request line
	std::istringstream stream(requestLine);

	if (!(stream >> _method >> _rawPath >> _version))
	{
		Logger::debug("Invalid request line: " + requestLine, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(400, "Invalid request line: " + requestLine, NULL, HttpResponse::FATAL_ERROR);
		return;
	}

	// Split the path and query string
	size_t queryPos = _rawPath.find('?');
	if (queryPos != std::string::npos)
	{
		_rawQueryString = _rawPath.substr(queryPos + 1);
		_rawPath = _rawPath.substr(0, queryPos);
	}
	_parseQueryParameters(_rawQueryString);

	// Decode the raw path
	_decodedPath = StrUtils::percentDecode(_rawPath);

	// Validation
	if (!_validateMethod(_method, response) || !_validateVersion(_version, response) ||
		!_validatePath(_decodedPath, response))
	{
		_uriState = URI_PARSING_ERROR;
		return;
	}

	_uriState = URI_PARSING_COMPLETE;
}

/*
** --------------------------------- SANITIZATION METHOD ----------------------------------
*/

// Helper: Resolve path with realpath and verify containment within root
// Returns true if successful, false if realpath fails or path escapes root
bool HttpURI::_resolvePathWithContainment(const std::string &path, const std::string &root, bool preserveTrailingSlash,
										  std::string &outResolved) const
{
	char resolvedPath[PATH_MAX];
	if (realpath(path.c_str(), resolvedPath) == NULL)
		return false;

	outResolved = resolvedPath;

	// Preserve trailing slash if requested and original had one
	if (preserveTrailingSlash && !outResolved.empty() && outResolved[outResolved.size() - 1] != '/')
		outResolved += "/";

	// Containment check
	return FileUtils::inRoot(root, outResolved);
}

void HttpURI::resolveURI(const Location *location, HttpResponse &response)
{
	// Get the root path from location
	const std::string *root = location->getRootPath();
	if (root == NULL || root->empty())
	{
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(500, "Location root path is not set", location, HttpResponse::FATAL_ERROR);
		return;
	}

	// Combine root + decoded path
	std::string fullPath = *root + "/" + _decodedPath;
	bool hasTrailingSlash = !_decodedPath.empty() && _decodedPath[_decodedPath.size() - 1] == '/';

	// First attempt: resolve full path
	if (_resolvePathWithContainment(fullPath, *root, hasTrailingSlash, _resolvedPath))
		return;

	// Check if containment failed (realpath succeeded but path escapes root)
	{
		char testPath[PATH_MAX];
		if (realpath(fullPath.c_str(), testPath) != NULL)
		{
			// realpath succeeded but containment failed
			_uriState = URI_PARSING_ERROR;
			response.setResponseDefaultBody(403, "Forbidden: Path escapes root", location, HttpResponse::ERROR);
			return;
		}
	}

	// If full path resolution failed, handle based on location type
	switch (location->getLocationType())
	{
	case Location::CGI:
	case Location::UPLOAD:
	{
		// fallback: resolve directory
		size_t lastSlash = fullPath.find_last_of('/');
		std::string directoryPath = (lastSlash != std::string::npos) ? fullPath.substr(0, lastSlash) : fullPath;

		if (directoryPath.empty())
			directoryPath = *root;

		std::string resolvedDir;
		if (!_resolvePathWithContainment(directoryPath, *root, true, resolvedDir))
		{
			// Check if it was a containment failure vs not found
			char testPath[PATH_MAX];
			if (realpath(directoryPath.c_str(), testPath) != NULL)
			{
				_uriState = URI_PARSING_ERROR;
				response.setResponseDefaultBody(403, "Forbidden: Path escapes root", location, HttpResponse::ERROR);
			}
			else
			{
				_uriState = URI_PARSING_ERROR;
				response.setResponseDefaultBody(404, "Not Found: Could not resolve path", location,
												HttpResponse::ERROR);
			}
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

		_resolvedPath = resolvedDir;
		return;
	}

	case Location::STATIC:
	case Location::REDIRECT:
	default:
		// For these types, no fallback — fail immediately
		_uriState = URI_PARSING_ERROR;
		response.setResponseDefaultBody(404, "Not Found: Could not resolve path", location, HttpResponse::ERROR);
		return;
	}
}

/*
** --------------------------------- ACCESSORS ----------------------------------
*/

// State accessor
HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

// Request line accessors
const std::string &HttpURI::getMethod() const
{
	return _method;
}

const std::string &HttpURI::getRawPath() const
{
	return _rawPath;
}

const std::string &HttpURI::getVersion() const
{
	return _version;
}

// URI versions accessors
const std::string &HttpURI::getDecodedPath() const
{
	return _decodedPath;
}

const std::string &HttpURI::getResolvedPath() const
{
	return _resolvedPath;
}

// Query parameters accessors
const std::string &HttpURI::getRawQueryString() const
{
	return _rawQueryString;
}

const std::string &HttpURI::getDecodedQueryString() const
{
	return _decodedQueryString;
}

const std::map<std::string, std::vector<std::string> > &HttpURI::getQueryParameters() const
{
	return _queryParameters;
}

/*
** --------------------------------- UTILS ----------------------------------
*/

void HttpURI::reset()
{
	// Parsing
	_uriState = URI_PARSING;
	_rawURISize = 0;

	// Request line
	_method.clear();
	_version.clear();

	// URI Versions
	_rawPath.clear();
	_decodedPath.clear();
	_resolvedPath.clear();

	// Query parameters
	_rawQueryString.clear();
	_decodedQueryString.clear();
	_queryParameters.clear();
}

size_t HttpURI::getRawURISize() const
{
	return _rawURISize;
}
