#include "../../includes/CGI/CgiResponse.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CgiResponse::CgiResponse() : _statusCode(200), _statusMessage("OK"), _isParsed(false), _isNPH(false)
{
}

CgiResponse::CgiResponse(const CgiResponse &other)
	: _headers(other._headers), _body(other._body), _statusCode(other._statusCode),
	  _statusMessage(other._statusMessage), _isParsed(other._isParsed), _isNPH(other._isNPH)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

CgiResponse::~CgiResponse()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

CgiResponse &CgiResponse::operator=(const CgiResponse &other)
{
	if (this != &other)
	{
		_headers = other._headers;
		_body = other._body;
		_statusCode = other._statusCode;
		_statusMessage = other._statusMessage;
		_isParsed = other._isParsed;
		_isNPH = other._isNPH;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

CgiResponse::ParseResult CgiResponse::parseOutput(const std::string &cgiOutput)
{
	clear();
	if (cgiOutput.empty())
	{
		Logger::log(Logger::ERROR, "CGI output is empty");
		return ERROR_MALFORMED_RESPONSE;
	}
	// Find the separator between headers and body (double CRLF or double LF)
	size_t headerEndPos = cgiOutput.find("\r\n\r\n");
	size_t separatorLength = 0;

	if (headerEndPos != std::string::npos)
		separatorLength = 4; // Length of "\r\n\r\n"
	else
	{
		headerEndPos = cgiOutput.find("\n\n");
		if (headerEndPos != std::string::npos)
			separatorLength = 2; // Length of "\n\n"
	}
	if (headerEndPos == std::string::npos)
	{
		// No header/body separator found - treat entire output as body
		_body = cgiOutput;
		setDefaultHeaders();
		_isParsed = true;
		return SUCCESS;
	}
	else
	{
		headerEndPos += separatorLength; // Move past the separator
		std::string header = cgiOutput.substr(0, headerEndPos);
		_body = cgiOutput.substr(headerEndPos);
		parseHeaders(header);
	}

	// Extract headers and body
	std::string headerSection = cgiOutput.substr(0, headerEndPos - separatorLength);
	_body = cgiOutput.substr(headerEndPos);

	// Check if this is NPH (Non-Parsed Header) mode
	if (headerSection.find("HTTP/") == 0)
	{
		_isNPH = true;
		size_t firstLineEnd = headerSection.find('\n');
		if (firstLineEnd != std::string::npos)
		{
			std::string statusLine = headerSection.substr(0, firstLineEnd);
			ParseResult result = parseStatusLine(statusLine);
			if (result != SUCCESS)
				return result;
			std::string remainingHeaders = headerSection.substr(firstLineEnd + 1);
			return parseHeaders(remainingHeaders);
		}
	}
	else
	{
		// Standard CGI mode - parse headers
		ParseResult result = parseHeaders(headerSection);
		if (result != SUCCESS)
			return result;
	}
	processSpecialHeaders();
	setDefaultHeaders();
	_isParsed = true;
	return SUCCESS;
}

void CgiResponse::populateHttpResponse(HttpResponse &httpResponse) const
{
	if (!_isParsed)
	{
		Logger::log(Logger::ERROR, "Cannot populate HttpResponse - CGI response not parsed");
		return;
	}
	httpResponse.setStatus(_statusCode, _statusMessage);

	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		httpResponse.setHeader(Header(it->first + ": " + it->second));
	}

	httpResponse.setBody(_body);
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

const std::map<std::string, std::string> &CgiResponse::getHeaders() const
{
	return _headers;
}

const std::string &CgiResponse::getBody() const
{
	return _body;
}

int CgiResponse::getStatusCode() const
{
	return _statusCode;
}

const std::string &CgiResponse::getStatusMessage() const
{
	return _statusMessage;
}

bool CgiResponse::isParsed() const
{
	return _isParsed;
}

bool CgiResponse::isNPH() const
{
	return _isNPH;
}

std::string CgiResponse::getHeader(const std::string &name) const
{
	std::string lowerName = toLowerCase(name);
	std::map<std::string, std::string>::const_iterator it = _headers.find(lowerName);
	if (it != _headers.end())
	{
		return it->second;
	}
	return "";
}

bool CgiResponse::hasHeader(const std::string &name) const
{
	std::string lowerName = toLowerCase(name);
	return _headers.find(lowerName) != _headers.end();
}

void CgiResponse::clear()
{
	_headers.clear();
	_body.clear();
	_statusCode = 200;
	_statusMessage = "OK";
	_isParsed = false;
	_isNPH = false;
}

/*
** --------------------------------- PRIVATE ----------------------------------
*/

CgiResponse::ParseResult CgiResponse::parseHeaders(const std::string &headerSection)
{
	std::istringstream headerStream(headerSection);
	std::string line;

	while (std::getline(headerStream, line))
	{
		// Remove carriage return if present
		if (!line.empty() && line[line.length() - 1] == '\r')
		{
			line.erase(line.length() - 1);
		}

		// Skip empty lines
		if (line.empty())
		{
			continue;
		}

		parseHeaderLine(line);
	}

	return SUCCESS;
}

CgiResponse::ParseResult CgiResponse::parseStatusLine(const std::string &statusLine)
{
	// Parse HTTP status line: "HTTP/1.1 200 OK"
	std::istringstream ss(statusLine);
	std::string httpVersion, statusCodeStr;

	if (!(ss >> httpVersion >> statusCodeStr))
	{
		Logger::log(Logger::ERROR, "Invalid HTTP status line: " + statusLine);
		return ERROR_INVALID_STATUS;
	}

	// Parse status code
	int statusCode = std::atoi(statusCodeStr.c_str());
	if (!isValidStatusCode(statusCode))
	{
		Logger::log(Logger::ERROR, "Invalid status code: " + statusCodeStr);
		return ERROR_INVALID_STATUS;
	}

	_statusCode = statusCode;

	// Extract status message (rest of the line)
	std::string statusMessage;
	std::getline(ss, statusMessage);
	_statusMessage = trim(statusMessage);

	if (_statusMessage.empty())
	{
		_statusMessage = getDefaultStatusMessage(_statusCode);
	}

	return SUCCESS;
}

void CgiResponse::parseHeaderLine(const std::string &line)
{
	size_t colonPos = line.find(':');
	if (colonPos == std::string::npos)
	{
		Logger::log(Logger::WARNING, "Invalid header line (no colon): " + line);
		return;
	}

	std::string name = trim(line.substr(0, colonPos));
	std::string value = trim(line.substr(colonPos + 1));

	if (name.empty())
	{
		Logger::log(Logger::WARNING, "Empty header name in line: " + line);
		return;
	}

	// Store header with lowercase name for case-insensitive access
	_headers[toLowerCase(name)] = value;
}

std::string CgiResponse::toLowerCase(const std::string &str) const
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::string CgiResponse::trim(const std::string &str) const
{
	size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
	{
		return "";
	}

	size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

bool CgiResponse::isValidStatusCode(int code) const
{
	return code >= 100 && code <= 599;
}

std::string CgiResponse::getDefaultStatusMessage(int code) const
{
	switch (code)
	{
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 204:
		return "No Content";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 304:
		return "Not Modified";
	case 400:
		return "Bad Request";
	case 401:
		return "Unauthorized";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 502:
		return "Bad Gateway";
	case 503:
		return "Service Unavailable";
	default:
		return "Unknown";
	}
}

void CgiResponse::processSpecialHeaders()
{
	// Handle Status header (CGI-specific)
	if (hasHeader("status"))
	{
		std::string statusHeader = getHeader("status");
		std::istringstream ss(statusHeader);
		std::string statusCodeStr;

		if (ss >> statusCodeStr)
		{
			int statusCode = std::atoi(statusCodeStr.c_str());
			if (isValidStatusCode(statusCode))
			{
				_statusCode = statusCode;

				// Extract status message if present
				std::string statusMessage;
				std::getline(ss, statusMessage);
				statusMessage = trim(statusMessage);

				if (!statusMessage.empty())
				{
					_statusMessage = statusMessage;
				}
				else
				{
					_statusMessage = getDefaultStatusMessage(_statusCode);
				}
			}
		}

		// Remove the Status header as it's not a standard HTTP header
		_headers.erase("status");
	}

	// Handle Location header for redirects
	if (hasHeader("location") && _statusCode == 200)
	{
		// If Location header is present but no status was set, default to 302
		_statusCode = 302;
		_statusMessage = "Found";
	}
}

void CgiResponse::setDefaultHeaders()
{
	// Always set Content-Length based on actual body length
	// This ensures correct Content-Length even if CGI script provided an
	// incorrect one or if no Content-Length was provided (EOF-based
	// termination)
		_headers["content-length"] = StrUtils::toString(_body.length());

	// Set Content-Type if not already set
	if (!hasHeader("content-type"))
	{
		_headers["content-type"] = "text/html";
	}
}

/* ************************************************************************** */
