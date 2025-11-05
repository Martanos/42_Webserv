#include "../../includes/HTTP/HttpRequest.hpp"
#include "../../includes/Core/Server.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/PerformanceMonitor.hpp"
#include "../../includes/HTTP/HttpBody.hpp"
#include "../../includes/HTTP/HttpHeaders.hpp"
#include "../../includes/HTTP/HttpURI.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

HttpRequest::HttpRequest()
{
	_parseState = PARSING_URI;
	_uri = HttpURI();
	_headers = HttpHeaders();
	_body = HttpBody();
	_potentialServers = NULL;
	_selectedServer = NULL;
}

HttpRequest::HttpRequest(const HttpRequest &src)
{
	*this = src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

HttpRequest::~HttpRequest()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

HttpRequest &HttpRequest::operator=(const HttpRequest &rhs)
{
	if (this != &rhs)
	{
		_uri = rhs._uri;
		_headers = rhs._headers;
		_body = rhs._body;
		_parseState = rhs._parseState;
		_potentialServers = rhs._potentialServers;
		_selectedServer = rhs._selectedServer;
	}
	return *this;
}

/*
** --------------------------------- Private utility methods METHODS
*----------------------------------
*/

bool HttpRequest::_identifyServer(HttpResponse &response)
{
	if (_potentialServers == NULL)
	{
		Logger::error("HttpRequest: No potential servers found", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponse(500, "Internal Server Error");
		return false;
	}
	const Header *hostHeader = _headers.getHeader("host");
	if (hostHeader == NULL)
	{
		Logger::error("HttpRequest: No host header found", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponse(400, "Bad Request");
		return false;
	}
	std::string hostValue = hostHeader->getValues()[0];
	size_t colonPos = hostValue.find(':');
	std::string hostName = (colonPos == std::string::npos) ? hostValue : hostValue.substr(0, colonPos);
	std::string port = (colonPos == std::string::npos) ? "80" : hostValue.substr(colonPos + 1);
	StrUtils::toLowerCase(hostName);
	for (std::vector<Server>::const_iterator it = _potentialServers->begin(); it != _potentialServers->end(); ++it)
	{
		if (it->hasServerName(hostName))
		{
			for (std::vector<SocketAddress>::const_iterator it2 = it->getSocketAddresses().begin();
				 it2 != it->getSocketAddresses().end(); ++it2)
			{
				if (it2->getPort() == static_cast<unsigned short>(std::atoi(port.c_str())))
				{
					_selectedServer = const_cast<Server *>(&(*it));
					return true;
				}
			}
		}
	}
	response.setResponse(404, "Not Found");
	Logger::error("HttpRequest: Server not found for host: " + hostValue, __FILE__, __LINE__, __PRETTY_FUNCTION__);
	return false;
}

/*
** --------------------------------- PARSING METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::parseBuffer(std::vector<char> &holdingBuffer, HttpResponse &response)
{
	PERF_SCOPED_TIMER(http_request_parsing);

	Logger::debug("HttpRequest: parseBuffer called, state: " + StrUtils::toString(_parseState) +
				  ", buffer size: " + StrUtils::toString(holdingBuffer.size()));

	// Continue parsing until complete or need more data
	while (_parseState != PARSING_COMPLETE && _parseState != PARSING_ERROR && !holdingBuffer.empty())
	{
		// Delegate to the appropriate parser
		switch (_parseState)
		{
		case PARSING_URI:
		{
			Logger::debug("HttpRequest: Parsing URI");
			_uri.parseBuffer(holdingBuffer, response);
			Logger::debug("HttpRequest: URI state: " + StrUtils::toString(_uri.getURIState()));
			switch (_uri.getURIState())
			{
			case HttpURI::URI_PARSING_COMPLETE:
			{
				Logger::debug("HttpRequest: URI parsing complete");
				Logger::debug("URI: " + _uri.getMethod() + " " + _uri.getURI() + " " + _uri.getVersion());
				_parseState = PARSING_HEADERS;
				Logger::debug("HttpRequest: Transitioned to PARSING_HEADERS");
				break;
			}
			case HttpURI::URI_PARSING_ERROR:
				Logger::error("HttpRequest: Request line parsing error");
				_parseState = PARSING_ERROR;
				break;
			default:
				return _parseState;
			}
			break;
		}
		case PARSING_HEADERS:
		{
			Logger::debug("HttpRequest: Starting headers parsing");
			_headers.parseBuffer(holdingBuffer, response, _body);
			Logger::debug("HttpRequest: Headers parsing state: " + StrUtils::toString(_headers.getHeadersState()));
			switch (_headers.getHeadersState())
			{
			case HttpHeaders::HEADERS_PARSING_COMPLETE:
			{
				if (!_identifyServer(response))
				{
					Logger::error("HttpRequest: Failed to identify server", __FILE__, __LINE__, __PRETTY_FUNCTION__);
					_parseState = PARSING_ERROR;
					break;
				}
				Logger::debug("HttpRequest: Headers parsing complete");
				switch (_body.getBodyType())
				{
				case HttpBody::BODY_TYPE_NO_BODY:
					_parseState = PARSING_COMPLETE;
					break;
				case HttpBody::BODY_TYPE_CONTENT_LENGTH:
				case HttpBody::BODY_TYPE_CHUNKED:
					_parseState = PARSING_BODY;
					break;
				default:
					Logger::error("HttpRequest: Invalid body type", __FILE__, __LINE__, __PRETTY_FUNCTION__);
					_parseState = PARSING_ERROR;
					break;
				}
				break;
			}
			case HttpHeaders::HEADERS_PARSING_ERROR:
				Logger::error("HttpRequest: Headers parsing error", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_parseState = PARSING_ERROR;
				break;
			case HttpHeaders::HEADERS_PARSING:
				Logger::debug("HttpRequest: Headers still parsing, need more data");
				return _parseState;
			}
			break;
		}
		case PARSING_BODY:
		{
			Logger::debug("HttpRequest: Parsing body");
			_body.parseBuffer(holdingBuffer, response);
			Logger::debug("HttpRequest: Body state: " + StrUtils::toString(_body.getBodyState()));
			switch (_body.getBodyState())
			{
			case HttpBody::BODY_PARSING_COMPLETE:
			{
				Logger::debug("HttpRequest: Body parsing complete");
				_parseState = PARSING_COMPLETE;
				Logger::debug("HttpRequest: Transitioned to PARSING_COMPLETE");
				break;
			}
			case HttpBody::BODY_PARSING:
				Logger::debug("HttpRequest: Body parsing in progress");
				_parseState = PARSING_BODY;
				break;
			case HttpBody::BODY_PARSING_ERROR:
				Logger::error("HttpRequest: Body parsing error", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_parseState = PARSING_ERROR;
				break;
			}
			break;
		}
		default:
			break;
		}
	}
	Logger::debug("HttpRequest: parseBuffer returning state: " + StrUtils::toString(_parseState));
	return _parseState;
}

void HttpRequest::sanitizeRequest(HttpResponse &response, const Server *server, const Location *location)
{
	_uri.sanitizeURI(server, location, response);
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

const std::vector<std::string> HttpRequest::getHeader(const std::string &name) const
{
	const Header *header = _headers.getHeader(name);
	if (header)
		return header->getValues();
	return std::vector<std::string>();
}

// Enhanced reset method
void HttpRequest::reset()
{
	_headers.reset();
	_body.reset();
	_body.setExpectedBodySize(0);
	_body.setBodyType(HttpBody::BODY_TYPE_NO_BODY);
	_parseState = PARSING_URI;
}

/*
** --------------------------------- MUTATOR METHODS
*----------------------------------
*/

void HttpRequest::setParseState(ParseState parseState)
{
	_parseState = parseState;
}
void HttpRequest::setSelectedServer(Server *selectedServer)
{
	_selectedServer = selectedServer;
}

void HttpRequest::setPotentialServers(const std::vector<Server> *potentialServers)
{
	_potentialServers = potentialServers;
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

// TODO: change to reference
HttpRequest::ParseState HttpRequest::getParseState() const
{
	return _parseState;
};

// URI accessors
std::string HttpRequest::getMethod() const
{
	return _uri.getMethod();
};

std::string HttpRequest::getUri() const
{
	return _uri.getURI();
};

std::string HttpRequest::getRawUri() const
{
	return _uri.getRawURI();
};

std::string HttpRequest::getVersion() const
{
	return _uri.getVersion();
};

std::map<std::string, std::vector<std::string> > HttpRequest::getHeaders() const
{
	std::map<std::string, std::vector<std::string> > result;
	const std::vector<Header> &headers = _headers.getHeaders();

	for (std::vector<Header>::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		result[it->getDirective()] = it->getValues();
	}

	return result;
};

std::string HttpRequest::getBodyData() const
{
	return _body.getRawBody();
};

HttpBody::BodyType HttpRequest::getBodyType() const
{
	return _body.getBodyType();
};

size_t HttpRequest::getMessageSize() const
{
	return _uri.getURIsize() + _headers.getHeadersSize() + _body.getBodySize();
};

bool HttpRequest::isUsingTempFile() const
{
	return _body.getIsUsingTempFile();
};

std::string HttpRequest::getTempFile() const
{
	return _body.getTempFilePath();
};

const std::vector<Server> *HttpRequest::getPotentialServers() const
{
	return _potentialServers;
};

Server *HttpRequest::getSelectedServer() const
{
	return _selectedServer;
};
