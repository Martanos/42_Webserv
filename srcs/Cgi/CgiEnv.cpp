#include "../../includes/Cgi/CgiEnv.hpp"
#include "../../includes/Config/Location.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include "../../includes/Wrappers/SocketAddress.hpp"
#include <cstdlib>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

CgiEnv::CgiEnv()
{
}

CgiEnv::CgiEnv(const CgiEnv &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

CgiEnv::~CgiEnv()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

CgiEnv &CgiEnv::operator=(const CgiEnv &rhs)
{
	if (this != &rhs)
	{
		_envVariables = rhs._envVariables;
	}
	return *this;
}

std::ostream &operator<<(std::ostream &os, const CgiEnv &env)
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

void CgiEnv::setEnv(const std::string &key, const std::string &value)
{
	_envVariables[key] = value;
}

const std::map<std::string, std::string> &CgiEnv::getEnvVariables() const
{
	return _envVariables;
}

std::string CgiEnv::getEnv(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _envVariables.find(key);
	if (it != _envVariables.end())
	{
		return it->second;
	}
	return "";
}

void CgiEnv::printEnv() const
{
	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin(); it != _envVariables.end(); ++it)
	{
		std::cout << it->first << "=" << it->second << std::endl;
	}
}

void CgiEnv::_transposeData(const std::string &pathToExecute, const HttpRequest &request, const Location *location)
{
	try
	{
		// Clear any previous environment
		_envVariables.clear();

		// Server information
		setEnv("SERVER_NAME", request.getSelectedServerHost());
		setEnv("SERVER_PORT", request.getSelectedServerPort());
		setEnv("SERVER_PROTOCOL", "HTTP/1.1");
		setEnv("SERVER_SOFTWARE", "webserv/1.0");
		setEnv("GATEWAY_INTERFACE", "CGI/1.1");

		// Request information
		setEnv("REQUEST_METHOD", request.getURI().getMethod());
		setEnv("QUERY_STRING", request.getURI().getDecodedQueryString());

		// Remote client information
		if (request.getRemoteAddress())
		{
			setEnv("REMOTE_ADDR", request.getRemoteAddress()->getHost());
			setEnv("REMOTE_PORT", request.getRemoteAddress()->getPortString());
		}
		setEnv("REQUEST_URI", request.getURI().getDecodedPath());

		// Body information
		switch (request.getBody().getBodyType())
		{
		case HttpBody::BODY_TYPE_CHUNKED:
		case HttpBody::BODY_TYPE_CONTENT_LENGTH:
		{
			setEnv("CONTENT_LENGTH", StrUtils::toString(request.getBody().getRawBodySize()));
			const Header *contentType = request.getHeaders().getHeader("content-type");
			if (contentType && !contentType->getValues().empty())
			{
				// Reconstruct full Content-Type with parameters (e.g., boundary for multipart)
				std::string fullContentType = contentType->getValues()[0];
				const std::vector<std::pair<std::string, std::string> > &params = contentType->getParameters();
				for (std::vector<std::pair<std::string, std::string> >::const_iterator it = params.begin();
					 it != params.end(); ++it)
				{
					fullContentType += "; " + it->first + "=" + it->second;
				}
				setEnv("CONTENT_TYPE", fullContentType);
			}
			break;
		}
		default:
			break;
		}

		// Script information and PATH_INFO extraction
		setEnv("SCRIPT_FILENAME", pathToExecute);

		// Extract PATH_INFO: the portion of URL path after the script name
		// E.g., for URL /cgi-bin/script.php/extra/path, PATH_INFO = /extra/path
		std::string pathInfo;
		std::string scriptName;
		const std::string &decodedPath = request.getURI().getDecodedPath();

		// Get the script filename (last component of pathToExecute)
		size_t lastSlash = pathToExecute.find_last_of('/');
		std::string scriptFilename =
			(lastSlash != std::string::npos) ? pathToExecute.substr(lastSlash + 1) : pathToExecute;

		// Find the script filename in the decoded URL path
		size_t scriptPos = decodedPath.find(scriptFilename);
		if (scriptPos != std::string::npos)
		{
			size_t scriptEnd = scriptPos + scriptFilename.length();
			// SCRIPT_NAME is the URL path up to and including the script
			scriptName = decodedPath.substr(0, scriptEnd);
			// PATH_INFO is everything after the script in the URL
			if (scriptEnd < decodedPath.length())
			{
				pathInfo = decodedPath.substr(scriptEnd);
				// PATH_INFO must start with '/' if present
				if (!pathInfo.empty() && pathInfo[0] != '/')
					pathInfo = "/" + pathInfo;
			}
		}
		else
		{
			// Fallback: use decoded path as script name, no PATH_INFO
			scriptName = decodedPath;
		}

		setEnv("SCRIPT_NAME", scriptName);
		setEnv("PATH_INFO", pathInfo);

		// PATH_TRANSLATED: filesystem path that PATH_INFO would map to
		if (!pathInfo.empty() && location && location->getRootPath())
		{
			setEnv("PATH_TRANSLATED", *location->getRootPath() + pathInfo);
		}

		// Convert HTTP headers to CGI environment variables
		const HttpHeaders &httpHeaders = request.getHeaders();
		const std::vector<Header> &headers = httpHeaders.getHeaders();
		for (std::vector<Header>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		{
			const std::vector<std::string> &values = it->getValues();
			if (values.empty())
				continue;
			std::string headerName = it->getDirective();
			// Skip headers that are handled separately or should not be passed
			if (headerName == "authorization" || headerName == "proxy-authorization" ||
				headerName == "content-length" || headerName == "content-type")
				continue;
			std::string cgiHeaderName = "HTTP_" + _convertHeaderNameToCgi(headerName);
			setEnv(cgiHeaderName, values[0]);
		}

		// System PATH
		const char *systemPath = std::getenv("PATH");
		if (systemPath)
			setEnv("PATH", systemPath);
		else
			setEnv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");

		Logger::debug("CgiEnv: Environment setup complete, total env count: " + StrUtils::toString(getEnvCount()),
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error(std::string("CgiEnv: Exception during transpose: ") + e.what(), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		throw;
	}
}

std::string CgiEnv::_convertHeaderNameToCgi(const std::string &headerName) const
{
	std::string result;
	result.reserve(headerName.length());
	for (size_t i = 0; i < headerName.length(); ++i)
	{
		char c = headerName[i];
		if (c == '-')
			result += '_';
		else if (c >= 'a' && c <= 'z')
			result += (c - 'a' + 'A');
		else if (c >= 'A' && c <= 'Z')
			result += c;
		else if (c >= '0' && c <= '9')
			result += c;
	}
	return result;
}

// TODO: refactor not necessary to allocate to heap
char **CgiEnv::getEnvArray() const
{
	char **envArray = new char *[_envVariables.size() + 1];
	size_t index = 0;
	for (std::map<std::string, std::string>::const_iterator it = _envVariables.begin(); it != _envVariables.end();
		 ++it, ++index)
	{
		const std::string &envString = it->first + "=" + it->second;
		envArray[index] = new char[envString.length() + 1];
		std::strcpy(envArray[index], envString.c_str());
	}
	envArray[index] = NULL;
	return envArray;
}

void CgiEnv::freeEnvArray(char **envArray) const
{
	if (!envArray)
		return;

	for (size_t i = 0; envArray[i] != NULL; ++i)
	{
		delete[] envArray[i];
	}
	delete[] envArray;
}

size_t CgiEnv::getEnvCount() const
{
	return _envVariables.size();
}

bool CgiEnv::hasEnv(const std::string &key) const
{
	return _envVariables.find(key) != _envVariables.end();
}

/* ************************************************************************** */
