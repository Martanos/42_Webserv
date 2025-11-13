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
	reset();
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
		response.setResponseDefaultBody(500, "Internal Server Error", NULL, NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	const Header *hostHeader = _headers.getHeader("host");
	if (hostHeader == NULL)
	{
		Logger::error("HttpRequest: No host header found", __FILE__, __LINE__, __PRETTY_FUNCTION__);
		response.setResponseDefaultBody(400, "No host header found", NULL, NULL, HttpResponse::FATAL_ERROR);
		return false;
	}
	std::string hostValue = (!hostHeader->getValues().empty()) ? hostHeader->getValues()[0] : "<none>";

	// Parse host and optional port from Host header
	std::string hostPart = hostValue;
	std::string portPart = "";
	if (!hostPart.empty() && hostPart[0] == '[')
	{
		// IPv6 in brackets: [::1]:8080 or [::1]
		size_t rb = hostPart.find(']');
		if (rb != std::string::npos)
		{
			std::string after = (rb + 1 < hostPart.size()) ? hostPart.substr(rb + 1) : std::string();
			std::string inside = hostPart.substr(1, rb - 1);
			hostPart = inside;
			if (!after.empty() && after[0] == ':')
				portPart = after.substr(1);
		}
	}
	else
	{
		// Split on last ':' only if it's the only colon (avoid IPv6 without brackets)
		size_t first = hostPart.find(':');
		size_t last = hostPart.rfind(':');
		if (first != std::string::npos && first == last)
		{
			hostPart = hostPart.substr(0, first);
			portPart = hostPart.size() < hostValue.size() ? hostValue.substr(first + 1) : std::string();
		}
	}
	// Normalize hostname to lowercase
	hostPart = StrUtils::toLowerCase(hostPart);
	_selectedServerHost = hostPart;
	_selectedServerPort = portPart; // may be empty if not provided

	// Try to find server by name (and port if provided)
	const std::vector<Server> &servers = *_potentialServers;
	for (std::vector<Server>::const_iterator it = servers.begin(); it != servers.end(); ++it)
	{
		if (it->hasServerName(_selectedServerHost))
		{
			if (!_selectedServerPort.empty())
			{
				for (std::vector<SocketAddress>::const_iterator it2 = it->getSocketAddresses().begin();
					 it2 != it->getSocketAddresses().end(); ++it2)
				{
					if (it2->getPortString() == _selectedServerPort)
					{
						_selectedServer = const_cast<Server *>(&(*it));
						return true;
					}
				}
			}
			else
			{
				// No port provided in Host header; accept name match on current listening socket
				_selectedServer = const_cast<Server *>(&(*it));
				return true;
			}
		}
	}
	// Fallback: use default server for this listening socket (first in list)
	if (!servers.empty())
	{
		Logger::warning("HttpRequest: No exact server_name/port match; falling back to default server", __FILE__,
						__LINE__, __PRETTY_FUNCTION__);
		_selectedServer = const_cast<Server *>(&servers.front());
		return true;
	}

	response.setResponseDefaultBody(404, "Matching server configuration not found", NULL, NULL,
									HttpResponse::FATAL_ERROR);
	Logger::debug("HttpRequest: Matching server configuration not found for host: " + hostValue, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
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
					  ", buffer size: " + StrUtils::toString(holdingBuffer.size()),
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);

	// Continue parsing until complete or need more data
	while (_parseState != PARSING_COMPLETE && _parseState != PARSING_ERROR && !holdingBuffer.empty())
	{
		// Delegate to the appropriate parser
		switch (_parseState)
		{
		case PARSING_URI:
		{
			Logger::debug("HttpRequest: Parsing URI", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_uri.parseBuffer(holdingBuffer, response);
			Logger::debug("HttpRequest: URI state: " + StrUtils::toString(_uri.getURIState()), __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			switch (_uri.getURIState())
			{
			case HttpURI::URI_PARSING_COMPLETE:
			{
				Logger::debug("HttpRequest: URI parsing complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				Logger::debug("URI: " + _uri.getMethod() + " " + _uri.getURI() + " " + _uri.getVersion(), __FILE__,
							  __LINE__, __PRETTY_FUNCTION__);
				_parseState = PARSING_HEADERS;
				Logger::debug("HttpRequest: Transitioned to PARSING_HEADERS", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				break;
			}
			case HttpURI::URI_PARSING_ERROR:
				Logger::error("HttpRequest: Request line parsing error", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_parseState = PARSING_ERROR;
				break;
			default:
				return _parseState;
			}
			break;
		}
		case PARSING_HEADERS:
		{
			Logger::debug("HttpRequest: Starting headers parsing", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_headers.parseBuffer(holdingBuffer, response, _body);
			Logger::debug("HttpRequest: Headers parsing state: " + StrUtils::toString(_headers.getHeadersState()),
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			switch (_headers.getHeadersState())
			{
			case HttpHeaders::HEADERS_PARSING_COMPLETE:
			{
				if (!_identifyServer(response))
				{
					const Header *hostHeader = _headers.getHeader("host");
					std::string hostValue =
						(hostHeader && !hostHeader->getValues().empty()) ? hostHeader->getValues()[0] : "<none>";
					Logger::error("HttpRequest: Failed to identify server for host value: " + hostValue, __FILE__,
								  __LINE__, __PRETTY_FUNCTION__);
					_parseState = PARSING_ERROR;
					break;
				}
				Logger::debug("HttpRequest: Headers parsing complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
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
				Logger::debug("HttpRequest: Headers still parsing, need more data", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				return _parseState;
			}
			break;
		}
		case PARSING_BODY:
		{
			Logger::debug("HttpRequest: Parsing body", __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_body.parseBuffer(holdingBuffer, response);
			Logger::debug("HttpRequest: Body state: " + StrUtils::toString(_body.getBodyState()), __FILE__, __LINE__,
						  __PRETTY_FUNCTION__);
			switch (_body.getBodyState())
			{
			case HttpBody::BODY_PARSING_COMPLETE:
			{
				Logger::debug("HttpRequest: Body parsing complete", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				_parseState = PARSING_COMPLETE;
				Logger::debug("HttpRequest: Transitioned to PARSING_COMPLETE", __FILE__, __LINE__, __PRETTY_FUNCTION__);
				break;
			}
			case HttpBody::BODY_PARSING:
				Logger::debug("HttpRequest: Body parsing incomplete, need more data", __FILE__, __LINE__,
							  __PRETTY_FUNCTION__);
				_parseState = PARSING_BODY;
				return _parseState;
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
	Logger::debug("HttpRequest: parseBuffer returning state: " + StrUtils::toString(_parseState), __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	return _parseState;
}

void HttpRequest::sanitizeRequest(HttpResponse &response, const Server *server, const Location *location)
{
	_uri.sanitizeURI(server, location, response);
	switch (_uri.getURIState())
	{
	case HttpURI::URI_PARSING_COMPLETE:
		break;
	case HttpURI::URI_PARSING_ERROR:
		_parseState = PARSING_ERROR;
		break;
	default:
		break;
	}
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
	_parseState = PARSING_URI;
	_potentialServers = NULL;
	_selectedServer = NULL;
	_uri.reset();
	_headers.reset();
	_body.reset();
	_selectedLocation = NULL;
	_internalRedirectDepth = 0;
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

void HttpRequest::setSelectedLocation(const Location *selectedLocation)
{
	_selectedLocation = const_cast<Location *>(selectedLocation);
}

void HttpRequest::setRemoteAddress(const SocketAddress *remoteAddress)
{
	_remoteAddress = const_cast<SocketAddress *>(remoteAddress);
}

/*
** --------------------------------- ACCESSOR METHODS
*----------------------------------
*/

HttpRequest::ParseState HttpRequest::getParseState() const
{
	return _parseState;
};

const std::map<std::string, std::vector<std::string> > &HttpRequest::getQueryParameters() const
{
	return _uri.getQueryParameters();
};

std::string HttpRequest::getQueryString() const
{
	return _uri.getQueryString();
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

size_t HttpRequest::getContentLength() const
{
	if (isUsingTempFile())
	{
		return (_body.getTempFd().getFileSize());
	}
	else
	{
		return (_body.getBodySize());
	}
}

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

Location *HttpRequest::getSelectedLocation() const
{
	return _selectedLocation;
};

SocketAddress *HttpRequest::getRemoteAddress() const
{
	return _remoteAddress;
};

const std::string &HttpRequest::getSelectedServerHost() const
{
	return _selectedServerHost;
};

const std::string &HttpRequest::getSelectedServerPort() const
{
	return _selectedServerPort;
};

const FileDescriptor &HttpRequest::getTempFd() const
{
	return _body.getTempFd();
};

/*
** --------------------------------- INTERNAL REDIRECT METHODS
*----------------------------------
*/

int HttpRequest::getInternalRedirectDepth() const
{
	return _internalRedirectDepth;
}

void HttpRequest::incrementInternalRedirectDepth()
{
	_internalRedirectDepth++;
}

void HttpRequest::resetInternalRedirectDepth()
{
	_internalRedirectDepth = 0;
}
