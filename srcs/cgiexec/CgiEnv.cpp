#include "../../includes/CGI/CgiEnv.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/Wrapper/SocketAddress.hpp"
#include <cstdlib>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CGIenv::CGIenv()
{
}

CGIenv::CGIenv(const CGIenv &src) : _envVariables(src._envVariables)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

CGIenv::~CGIenv()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

CGIenv &CGIenv::operator=(const CGIenv &rhs)
{
	if (this != &rhs)
	{
		_envVariables = rhs._envVariables;
	}
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void CGIenv::setEnv(const std::string &key, const std::string &value)
{
	_envVariables.insert(std::make_pair(key, value));
}

std::string CGIenv::getEnv(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _envVariables.find(key);
	if (it != _envVariables.end())
	{
		return it->second;
	}
	return "";
}

void CGIenv::printEnv() const
{
	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin(); it != _envVariables.end(); ++it)
	{
		std::cout << it->first << "=" << it->second << std::endl;
	}
}

void CGIenv::_transposeData(const HttpRequest &request, const Server *server, const Location *location)
{
	// Server information
	setEnv("SERVER_NAME", request.getSelectedServerHost());
	setEnv("SERVER_PORT", request.getSelectedServerPort());
	setEnv("REMOTE_ADDR", request.getRemoteAddress()->getHost());
	setEnv("REMOTE_PORT", request.getRemoteAddress()->getPortString());

	// Request information
	setEnv("REQUEST_URI", request.getRawUri());
	setEnv("REQUEST_METHOD", request.getMethod());
	setEnv("REQUEST_PATH", request.getSelectedLocation()->getPath());
	setEnv("QUERY_STRING", request.getQueryString());

	// Add additional server-specific environment variables
	setEnv("SERVER_SOFTWARE", "webserv/1.0");
	setEnv("SERVER_PROTOCOL", "HTTP/1.1");
	setEnv("GATEWAY_INTERFACE", "CGI/1.1");

	// TODO: Implement PATH_INFO and SCRIPT_NAME

	// Translate HTTP headers to CGI environment variables
	const std::map<std::string, std::vector<std::string> > &headers = request.getHeaders();

	for (std::map<std::string, std::vector<std::string> >::const_iterator it = headers.begin(); it != headers.end();
		 ++it)
	{
		if (!it->second.empty())
		{
			// Convert header name to CGI format: HTTP_HEADER_NAME
			std::string cgiHeaderName = "HTTP_" + convertHeaderNameToCgi(it->first);
			setEnv(cgiHeaderName, it->second[0]); // Use first value if multiple
		}
	}
}

void CGIenv::setupFromRequest(const HttpRequest &request, const Server *server, const Location *location,
							  const std::string &scriptPath)
{
	// Parse SCRIPT_NAME and PATH_INFO (requires location context but derived
	// from URI)
	if (location)
	{
		std::string locationPath = location->getPath();
		std::string scriptName = locationPath;
		std::string pathInfo = "";

		// If URI is longer than location path, the extra part is PATH_INFO
		if (uri.length() > locationPath.length() && uri.substr(0, locationPath.length()) == locationPath)
		{
			pathInfo = uri.substr(locationPath.length());
			if (!pathInfo.empty() && pathInfo[0] != '/')
			{
				pathInfo = "/" + pathInfo;
			}
		}

		setEnv("SCRIPT_NAME", scriptName);
		setEnv("PATH_INFO", pathInfo);
	}

	// Script filename (provided parameter)
	setEnv("SCRIPT_FILENAME", scriptPath);

	// Content information from HTTP headers
	if (request.getMethod() == "POST")
	{
		std::vector<std::string> contentType = request.getHeader("content-type");
		if (!contentType.empty())
		{
			setEnv("CONTENT_TYPE", contentType[0]);
		}

		setEnv("CONTENT_LENGTH", StrUtils::toString(request.getBodyData().length()));
	}
	else
	{
		setEnv("CONTENT_LENGTH", "0");
	}

	// Convert HTTP headers to CGI environment variables
	setupHttpHeaders(request);

	// Derive SERVER_NAME and SERVER_PORT from Host header when provided
	std::vector<std::string> hostHeader = request.getHeader("host");
	if (!hostHeader.empty())
	{
		std::string hostValue = hostHeader[0];
		size_t colonPos = hostValue.find(':');
		if (colonPos == std::string::npos)
		{
			setEnv("SERVER_NAME", hostValue);
		}
		else
		{
			setEnv("SERVER_NAME", hostValue.substr(0, colonPos));
			setEnv("SERVER_PORT", hostValue.substr(colonPos + 1));
		}
	}
}

std::string CGIenv::convertHeaderNameToCgi(const std::string &headerName) const
{
	std::string result;
	result.reserve(headerName.length());

	for (size_t i = 0; i < headerName.length(); ++i)
	{
		char c = headerName[i];
		if (c == '-')
		{
			result += '_';
		}
		else if (c >= 'a' && c <= 'z')
		{
			result += (c - 'a' + 'A'); // Convert to uppercase
		}
		else if (c >= 'A' && c <= 'Z')
		{
			result += c;
		}
		else if (c >= '0' && c <= '9')
		{
			result += c;
		}
		// Skip other characters
	}

	return result;
}

char **CGIenv::getEnvArray() const
{
	// Allocate array of char* pointers
	char **envArray = new char *[_envVariables.size() + 1];
	size_t index = 0;

	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin(); it != _envVariables.end();
		 ++it, ++index)
	{
		std::string envString = it->first + "=" + it->second;
		envArray[index] = new char[envString.length() + 1];
		std::strcpy(envArray[index], envString.c_str());
	}

	envArray[index] = NULL; // Null-terminate the array
	return envArray;
}

void CGIenv::freeEnvArray(char **envArray) const
{
	if (!envArray)
		return;

	for (size_t i = 0; envArray[i] != NULL; ++i)
	{
		delete[] envArray[i];
	}
	delete[] envArray;
}

size_t CGIenv::getEnvCount() const
{
	return _envVariables.size();
}

bool CGIenv::hasEnv(const std::string &key) const
{
	return _envVariables.find(key) != _envVariables.end();
}

/* ************************************************************************** */
