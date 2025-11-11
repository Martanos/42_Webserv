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

CGIenv::CGIenv(const CGIenv &src)
{
	*this = src;
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

std::ostream &operator<<(std::ostream &os, const CGIenv &env)
{
	for (std::map<std::string, std::string>::const_iterator it = env.getEnvVariables().begin();
		 it != env.getEnvVariables().end(); ++it)
	{
		os << it->first << "=" << it->second << std::endl;
	}
	return os;
}
/*
** --------------------------------- METHODS ----------------------------------
*/

void CGIenv::setEnv(const std::string &key, const std::string &value)
{
	_envVariables[key] = value;
}

const std::map<std::string, std::string> &CGIenv::getEnvVariables() const
{
	return _envVariables;
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
	// RFC 3875 MUST variables - Server information
	setEnv("SERVER_NAME", request.getSelectedServerHost());
	setEnv("SERVER_PORT", request.getSelectedServerPort());
	setEnv("SERVER_PROTOCOL", "HTTP/1.1");
	setEnv("SERVER_SOFTWARE", "webserv/1.0");
	setEnv("GATEWAY_INTERFACE", "CGI/1.1");

	// RFC 3875 MUST variables - Request information
	setEnv("REQUEST_METHOD", request.getMethod());
	setEnv("QUERY_STRING", request.getQueryString());

	// RFC 3875 MUST variables - Client information
	setEnv("REMOTE_ADDR", request.getRemoteAddress()->getHost());

	// Non-standard but useful
	setEnv("REMOTE_PORT", request.getRemoteAddress()->getPortString());
	setEnv("REQUEST_URI", request.getRawUri());

	// Content length (RFC states only if body is present)
	switch (request.getBodyType())
	{
	case HttpBody::BODY_TYPE_CHUNKED:
	case HttpBody::BODY_TYPE_CONTENT_LENGTH:
	{
		setEnv("CONTENT_LENGTH", StrUtils::toString(request.getContentLength()));
		std::vector<std::string> contentType = request.getHeader("content-type");
		if (!contentType.empty())
			setEnv("CONTENT_TYPE", contentType[0]);
		break;
	}
	default:
		break;
	}

	// Script name and path info
	std::string scriptName = CgiHandler::resolveCgiScriptPath(request.getUri(), server, location);
	setEnv("SCRIPT_NAME", scriptName);
	setEnv("SCRIPT_FILENAME", scriptName);

	// Translate HTTP headers to CGI environment variables
	const std::map<std::string, std::vector<std::string> > &headers = request.getHeaders();

	for (std::map<std::string, std::vector<std::string> >::const_iterator it = headers.begin(); it != headers.end();
		 ++it)
	{
		if (!it->second.empty())
		{
			std::string headerName = it->first;

			// Skip authentication headers per RFC 3875 Section 4.1.18
			if (headerName == "authorization" || headerName == "proxy-authorization")
			{
				continue; // Don't pass to CGI
			}

			// Skip headers already available as meta-variables
			if (headerName == "content-length" || headerName == "content-type")
			{
				continue; // Already set as CONTENT_LENGTH/CONTENT_TYPE
			}

			std::string cgiHeaderName = "HTTP_" + _convertHeaderNameToCgi(headerName);
			setEnv(cgiHeaderName, it->second[0]);
		}
	}
}

std::string CGIenv::_convertHeaderNameToCgi(const std::string &headerName) const
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
	}

	return result;
}

// TODO: refactor not necessary to allocate to heap
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
