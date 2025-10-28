#include "../../includes/HttpURI.hpp"
#include "../../includes/HttpResponse.hpp"
#include "../../includes/Logger.hpp"
#include "../../includes/StringUtils.hpp"
#include <algorithm>
#include <sstream>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

/*
** --------------------------------- METHODS ----------------------------------
*/

int HttpURI::parseBuffer(RingBuffer &buffer, HttpResponse &response)
{
    if (_uriState == URI_PARSING_COMPLETE)
        return URI_PARSING_COMPLETE;

    if (_uriState == URI_PARSING_ERROR)
        return URI_PARSING_ERROR;

    // Peek instead of transfer — don't consume all data
    std::string peeked;
    buffer.peekBuffer(peeked, buffer.readable());

    // Find the end of the request line (CRLF or LF)
    size_t lineEnd = peeked.find("\r\n");
    size_t lineLen = std::string::npos;

    if (lineEnd != std::string::npos)
        lineLen = lineEnd + 2;
    else {
        lineEnd = peeked.find('\n');
        if (lineEnd == std::string::npos)
            return URI_PARSING; // still incomplete
        lineLen = lineEnd + 1;
    }

    // Read exactly the request line (consume only that much)
    std::string requestLine;
    buffer.readBuffer(requestLine, lineLen);

    // Trim CRLF or LF manually
    if (requestLine.size() >= 2 &&
        requestLine[requestLine.size() - 2] == '\r' &&
        requestLine[requestLine.size() - 1] == '\n')
    {
        requestLine.erase(requestLine.size() - 2);
    }
    else if (!requestLine.empty() &&
             requestLine[requestLine.size() - 1] == '\n')
    {
        requestLine.erase(requestLine.size() - 1);
    }

    Logger::debug("HttpURI: Parsed request line: " + requestLine);

    std::istringstream stream(requestLine);
    std::string method, uri, version;
    if (!(stream >> method >> uri >> version))
    {
        Logger::log(Logger::ERROR, "Invalid request line: " + requestLine);
        _uriState = URI_PARSING_ERROR;
        response.setStatus(400, "Bad Request");
        return URI_PARSING_ERROR;
    }

    // Validate method (C++98-style vector initialization)
    std::vector<std::string> validMethods;
    validMethods.push_back("GET");
    validMethods.push_back("POST");
    validMethods.push_back("DELETE");

    bool methodValid = false;
    for (std::vector<std::string>::const_iterator it = validMethods.begin();
         it != validMethods.end(); ++it)
    {
        if (*it == method)
        {
            methodValid = true;
            break;
        }
    }

    if (!methodValid)
    {
        Logger::log(Logger::ERROR, "Unsupported HTTP method: " + method);
        _uriState = URI_PARSING_ERROR;
        response.setStatus(501, "Not Implemented");
        return URI_PARSING_ERROR;
    }

    // Validate URI
    if (uri.empty() || uri[0] != '/')
    {
        Logger::log(Logger::ERROR, "Invalid URI: " + uri);
        _uriState = URI_PARSING_ERROR;
        response.setStatus(400, "Bad Request");
        return URI_PARSING_ERROR;
    }

    // Validate version
    if (version != "HTTP/1.1" && version != "HTTP/1.0")
    {
        Logger::log(Logger::ERROR, "Unsupported HTTP version: " + version);
        _uriState = URI_PARSING_ERROR;
        response.setStatus(505, "HTTP Version Not Supported");
        return URI_PARSING_ERROR;
    }

    // Store parsed values
    _method = method;
    _uri = uri;
    _version = version;

    Logger::debug("HttpURI: Request line OK (" + method + " " + uri + " " + version + ")");
    _uriState = URI_PARSING_COMPLETE;

    Logger::debug("HttpURI: Remaining readable in main buffer after URI parse = " +
                  StringUtils::toString(buffer.readable()));

    return URI_PARSING_COMPLETE;
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

HttpURI::URIState HttpURI::getURIState() const
{
	return _uriState;
}

RingBuffer &HttpURI::getRawURI()
{
	return _rawURI;
}

std::string HttpURI::getMethod() const
{
	return _method;
}

std::string HttpURI::getURI() const
{
	return _uri;
}

std::string HttpURI::getVersion() const
{
	return _version;
}

/*
** --------------------------------- MUTATORS --------------------------------
*/

void HttpURI::setURIState(URIState uriState)
{
	_uriState = uriState;
}

void HttpURI::setRawURI(RingBuffer &rawURI)
{
	_rawURI = rawURI;
}

void HttpURI::setMethod(const std::string &method)
{
	_method = method;
}

void HttpURI::setURI(const std::string &uri)
{
	_uri = uri;
}

void HttpURI::setVersion(const std::string &version)
{
	_version = version;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void HttpURI::reset()
{
	_uriState = URI_PARSING;
	_rawURI.clear();
	_method = "";
	_uri = "";
	_version = "";
}