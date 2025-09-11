#include "CgiEnv.hpp"
#include "HttpRequest.hpp"
#include "Location.hpp"
#include "Server.hpp"
#include "StringUtils.hpp"
#include "Logger.hpp"
#include <cstdlib>
#include <sstream>

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
	_envVariables[key] = value;
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
	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin();
		 it != _envVariables.end(); ++it)
	{
		std::cout << it->first << "=" << it->second << std::endl;
	}
}

void CGIenv::copyDataFromServer(const ServerConfig &server, const LocationConfig &location)
{
	// Server information
	const std::vector<std::string> &serverNames = server.getServerNames();
	if (!serverNames.empty())
		setEnv("SERVER_NAME", serverNames[0]); // Use first server name
	else
		setEnv("SERVER_NAME", "localhost");
	
	// Get port from server configuration
	const std::vector<std::pair<std::string, unsigned short> > &hostsPorts = server.getHosts_ports();
	if (!hostsPorts.empty())
		setEnv("SERVER_PORT", StringUtils::toString(hostsPorts[0].second)); // Use first port
	else
		setEnv("SERVER_PORT", "80"); // Default port
	
	// Document root from location or server
	std::string root = location.getRoot();
	if (!root.empty())
		setEnv("DOCUMENT_ROOT", root);
	else
		setEnv("DOCUMENT_ROOT", server.getRoot());
	
	// Add custom CGI parameters from location configuration
	const std::map<std::string, std::string> &cgiParams = location.getCgiParams();
	for (std::map<std::string, std::string>::const_iterator it = cgiParams.begin();
		 it != cgiParams.end(); ++it)
		setEnv(it->first, it->second);
}

void CGIenv::setupFromRequest(const HttpRequest &request, 
							  const Server *server, 
							  const Location *location,
							  const std::string &scriptPath)
{
	// Basic CGI environment variables from HTTP standard
	setEnv("REQUEST_METHOD", request.getMethod());
	setEnv("SERVER_SOFTWARE", "webserv/1.0");
	setEnv("SERVER_PROTOCOL", "HTTP/1.1");
	setEnv("GATEWAY_INTERFACE", "CGI/1.1");

	// Request URI and script information (derived from HttpRequest)
	std::string uri = request.getUri();
	setEnv("REQUEST_URI", uri);
	
	// Parse SCRIPT_NAME and PATH_INFO (requires location context but derived from URI)
	if (location)
	{
		std::string locationPath = location->getPath();
		std::string scriptName = locationPath;
		std::string pathInfo = "";
		
		// If URI is longer than location path, the extra part is PATH_INFO
		if (uri.length() > locationPath.length() && 
			uri.substr(0, locationPath.length()) == locationPath)
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

	// Query string (parsed from URI)
	size_t queryPos = uri.find('?');
	if (queryPos != std::string::npos)
	{
		setEnv("QUERY_STRING", uri.substr(queryPos + 1));
	}
	else
	{
		setEnv("QUERY_STRING", "");
	}

	// Content information from HTTP headers
	if (request.getMethod() == "POST")
	{
		std::vector<std::string> contentType = request.getHeader("content-type");
		if (!contentType.empty())
		{
			setEnv("CONTENT_TYPE", contentType[0]);
		}
		
		setEnv("CONTENT_LENGTH", StringUtils::toString(request.getBody().length()));
	}
	else
	{
		setEnv("CONTENT_LENGTH", "0");
	}

	// Convert HTTP headers to CGI environment variables
	setupHttpHeaders(request);

	// Remote address (simplified - would need actual client info)
	// This should ideally come from the connection context, not server config
	setEnv("REMOTE_ADDR", "127.0.0.1");
	setEnv("REMOTE_HOST", "localhost");
	
	// Suppress unused parameter warnings
	(void)server;
}

void CGIenv::setupHttpHeaders(const HttpRequest &request)
{
	// Get all headers from the request
	const std::map<std::string, std::vector<std::string> > &headers = request.getHeaders();
	
	for (std::map<std::string, std::vector<std::string> >::const_iterator it = headers.begin();
		 it != headers.end(); ++it)
	{
		if (!it->second.empty())
		{
			// Convert header name to CGI format: HTTP_HEADER_NAME
			std::string cgiHeaderName = "HTTP_" + convertHeaderNameToCgi(it->first);
			setEnv(cgiHeaderName, it->second[0]); // Use first value if multiple
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
	char **envArray = new char*[_envVariables.size() + 1];
	size_t index = 0;
	
	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin();
		 it != _envVariables.end(); ++it, ++index)
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

void CGIenv::removeEnv(const std::string &key)
{
	_envVariables.erase(key);
}

void CGIenv::clear()
{
	_envVariables.clear();
}

/* ************************************************************************** */
