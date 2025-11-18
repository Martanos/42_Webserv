#include "../../includes/Cgi/CgiEnv.hpp"
#include "../../includes/Config/Location.hpp"
#include "../../includes/Config/Server.hpp"
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

void CgiEnv::_transposeData(const HttpRequest &request, const Server *server, const Location *location)
{
	const HttpURI &uri = request.getURI();
	const HttpHeaders &headersRef = request.getHeaders();
	const HttpBody &body = request.getBody();
	std::string rawUri = uri.getRawPath();
	if (!uri.getRawQueryString().empty())
		rawUri += "?" + uri.getRawQueryString();

	Logger::debug("CgiEnv: Begin transpose for raw URI: " + rawUri, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	try
	{
		setEnv("SERVER_NAME", request.getSelectedServerHost());
		setEnv("SERVER_PORT", request.getSelectedServerPort());
		setEnv("SERVER_PROTOCOL", "HTTP/1.1");
		setEnv("SERVER_SOFTWARE", "webserv/1.0");
		setEnv("GATEWAY_INTERFACE", "CGI/1.1");

		setEnv("REQUEST_METHOD", uri.getMethod());
		setEnv("QUERY_STRING", uri.getRawQueryString());

		if (request.getRemoteAddress())
		{
			setEnv("REMOTE_ADDR", request.getRemoteAddress()->getHost());
			setEnv("REMOTE_PORT", request.getRemoteAddress()->getPortString());
		}
		setEnv("REQUEST_URI", rawUri);

		switch (body.getBodyType())
		{
		case HttpBody::BODY_TYPE_CHUNKED:
		case HttpBody::BODY_TYPE_CONTENT_LENGTH:
		{
			setEnv("CONTENT_LENGTH", StrUtils::toString(body.getRawBodySize()));
			const Header *contentType = headersRef.getHeader("content-type");
			if (contentType && !contentType->getValues().empty())
				setEnv("CONTENT_TYPE", contentType->getValues()[0]);
			break;
		}
		default:
			break;
		}

		std::string cleanUri = StrUtils::sanitizeUriPath(uri.getResolvedPath());
		const std::string *basePath = NULL;
		if (location && location->getRootPath() && !location->getRootPath()->empty())
			basePath = location->getRootPath();
		else if (server && server->getRootPath() && !server->getRootPath()->empty())
			basePath = server->getRootPath();

		std::string scriptPath = cleanUri;
		if (basePath && !basePath->empty())
		{
			std::string normalizedBase = *basePath;
			if (!normalizedBase.empty() && normalizedBase[normalizedBase.size() - 1] == '/')
				normalizedBase.erase(normalizedBase.size() - 1);
			scriptPath = normalizedBase + cleanUri;
		}
		setEnv("SCRIPT_NAME", cleanUri);
		setEnv("SCRIPT_FILENAME", scriptPath);
		Logger::debug("CgiEnv: SCRIPT_NAME=" + cleanUri + " SCRIPT_FILENAME=" + scriptPath, __FILE__, __LINE__,
					  __PRETTY_FUNCTION__);

		const std::vector<Header> &headers = headersRef.getHeaders();
		for (std::vector<Header>::const_iterator it = headers.begin(); it != headers.end(); ++it)
		{
			const std::vector<std::string> &values = it->getValues();
			if (values.empty())
				continue;
			std::string headerName = it->getDirective();
			if (headerName == "authorization" || headerName == "proxy-authorization" ||
				headerName == "content-length" || headerName == "content-type")
				continue;
			std::string cgiHeaderName = "HTTP_" + _convertHeaderNameToCgi(headerName);
			setEnv(cgiHeaderName, values[0]);
		}

		const char *systemPath = std::getenv("PATH");
		if (systemPath)
			setEnv("PATH", systemPath);
		else
			setEnv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin");

		Logger::debug("CgiEnv: Header variables set, total env count: " + StrUtils::toString(getEnvCount()), __FILE__,
					  __LINE__, __PRETTY_FUNCTION__);
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
