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

void CgiEnv::_transposeData(const HttpRequest &request, const Server *server, const Location *location)
{
	Logger::debug("CgiEnv: Begin transpose for raw URI: " + request.getRawUri());
	try
	{
		// Server info
		Logger::debug("CgiEnv: Setting SERVER_NAME");
		setEnv("SERVER_NAME", request.getSelectedServerHost());
		Logger::debug("CgiEnv: Setting SERVER_PORT");
		setEnv("SERVER_PORT", request.getSelectedServerPort());
		Logger::debug("CgiEnv: Setting SERVER_PROTOCOL");
		setEnv("SERVER_PROTOCOL", "HTTP/1.1");
		Logger::debug("CgiEnv: Setting SERVER_SOFTWARE");
		setEnv("SERVER_SOFTWARE", "webserv/1.0");
		Logger::debug("CgiEnv: Setting GATEWAY_INTERFACE");
		setEnv("GATEWAY_INTERFACE", "CGI/1.1");

		// Request info
		Logger::debug("CgiEnv: Setting REQUEST_METHOD");
		setEnv("REQUEST_METHOD", request.getMethod());
		Logger::debug("CgiEnv: Setting QUERY_STRING");
		setEnv("QUERY_STRING", request.getQueryString());

		// Client info
		if (request.getRemoteAddress())
		{
			Logger::debug("CgiEnv: Setting REMOTE_ADDR");
			setEnv("REMOTE_ADDR", request.getRemoteAddress()->getHost());
			Logger::debug("CgiEnv: Setting REMOTE_PORT");
			setEnv("REMOTE_PORT", request.getRemoteAddress()->getPortString());
		}
		Logger::debug("CgiEnv: Setting REQUEST_URI");
		setEnv("REQUEST_URI", request.getRawUri());
		Logger::debug("CgiEnv: Core meta variables set");

		// Body meta
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

		// Resolve script path
		std::string cleanUri = StrUtils::sanitizeUriPath(request.getRawUri());
		std::string scriptPath;
		if (location && !location->getCgiPath().empty())
		{
			std::string base = location->getCgiPath();
			std::string locPrefix = location->getPath();
			if (!locPrefix.empty() && locPrefix[locPrefix.size() - 1] != '/')
				locPrefix += "/";
			std::string tail = cleanUri;
			if (cleanUri.find(locPrefix) == 0)
				tail = cleanUri.substr(locPrefix.size());
			while (!tail.empty() && tail[0] == '/')
				tail.erase(0, 1);
			scriptPath = base + "/" + tail;
		}
		else if (location && !location->getRoot().empty())
		{
			scriptPath = location->getRoot() + cleanUri;
		}
		else if (server && !server->getRootPath().empty())
		{
			scriptPath = server->getRootPath() + cleanUri;
		}
		else
		{
			scriptPath = cleanUri;
		}
		setEnv("SCRIPT_NAME", cleanUri);
		setEnv("SCRIPT_FILENAME", scriptPath);
		Logger::debug("CgiEnv: SCRIPT_NAME=" + cleanUri + " SCRIPT_FILENAME=" + scriptPath);

		// Headers
		const std::map<std::string, std::vector<std::string> > &headers = request.getHeaders();
		for (std::map<std::string, std::vector<std::string> >::const_iterator it = headers.begin(); it != headers.end();
			 ++it)
		{
			if (it->second.empty())
				continue;
			std::string headerName = it->first;
			if (headerName == "authorization" || headerName == "proxy-authorization")
				continue;
			if (headerName == "content-length" || headerName == "content-type")
				continue;
			std::string cgiHeaderName = "HTTP_" + _convertHeaderNameToCgi(headerName);
			setEnv(cgiHeaderName, it->second[0]);
		}

		// Add minimal shell environment for CGI (PATH for interpreter resolution)
		const char *systemPath = std::getenv("PATH");
		if (systemPath)
			setEnv("PATH", systemPath);
		else
			setEnv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");

		Logger::debug("CgiEnv: Header variables set, total env count: " + StrUtils::toString(getEnvCount()));
	}
	catch (const std::exception &e)
	{
		Logger::error(std::string("CgiEnv: Exception during transpose: ") + e.what(), __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);
		throw; // rethrow so caller can handle (will surface as 500)
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
