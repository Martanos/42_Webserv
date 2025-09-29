#include "../../includes/HttpURI.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/StringUtils.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpURI::HttpURI()
{
	_uriState = URI_PARSING;
	_method = "";
	_uri = "";
	_version = "";
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
		_uri = other._uri;
		_version = other._version;
	}
	return *this;
}
/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::parseBuffer(std::vector<char> &buffer, HttpResponse &response)
{
	// Scan buffer for CLRF
	const char *pattern = "\r\n\r\n";
	std::vector<char>::iterator it = std::search(buffer.begin(), buffer.end(), pattern, pattern + 4);
	if (it == buffer.end())
	{
		// If it can't be found check that the buffer has not currently exceeded the size limit of a header
		if (buffer.size() > static_cast<size_t>(sysconf(_SC_PAGESIZE) * 4))
		{
			response.setStatus(413, "Request Header Fields Too Large");
			Logger::log(Logger::ERROR, "Header size limit exceeded");
			_uriState = URI_PARSING_ERROR;
		}
		else
			_uriState = URI_PARSING;
		return;
	}

	// Extract request line up to the CLRF
	std::string requestLine;
	requestLine.assign(buffer.begin(), it);

	// Clear buffer up to the CLRF
	buffer.erase(buffer.begin(), it + 3);

	// Parse request line
	std::istringstream stream(requestLine);

	if (!(stream >> _method >> _uri >> _version))
	{
		Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// TODO: Move this to a constant
	// Validate method
	std::vector<std::string> validMethods;
	validMethods.push_back("GET");
	validMethods.push_back("POST");
	validMethods.push_back("DELETE");

	if (std::find(validMethods.begin(), validMethods.end(), _method) == validMethods.end())
	{
		Logger::log(Logger::ERROR, "Unsupported HTTP method: " + _method);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(501, "Not Implemented");
		return;
	}

	// Validate URI
	if (_uri.empty() || _uri[0] != '/')
	{
		Logger::log(Logger::ERROR, "Invalid URI: " + _uri);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(400, "Bad Request");
		return;
	}

	// Validate version
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
	{
		Logger::log(Logger::ERROR, "Unsupported HTTP version: " + _version);
		_uriState = URI_PARSING_ERROR;
		response.setStatus(505, "HTTP Version Not Supported");
		return;
	}

	Logger::debug("HttpURI: Parsed request line: " + _method + " " + _uri + " " + _version);
	_uriState = URI_PARSING_COMPLETE;
	return;
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

std::string &HttpURI::getMethod()
{
	return _method;
}

std::string &HttpURI::getURI()
{
	return _uri;
}

std::string &HttpURI::getVersion()
{
	return _version;
}

const std::string &HttpURI::getMethod() const
{
	return _method;
}

const std::string &HttpURI::getURI() const
{
	return _uri;
}

const std::string &HttpURI::getVersion() const
{
	return _version;
}

HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_method = "";
	_uri = "";
	_version = "";
}