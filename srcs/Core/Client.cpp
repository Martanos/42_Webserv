#include "../../includes/Core/Client.hpp"
#include "../../includes/Cgi/CgiHandler.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Http/HTTP.hpp"
#include "../../includes/Http/HttpRequest.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include "../../includes/MethodHandlers/MethodHandlerFactory.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <cstdio>
#include <sys/epoll.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// TODO: Remove DefaultStatus map should handle this
namespace {
std::string getRedirectReasonPhrase(int statusCode) {
  switch (statusCode) {
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 303:
    return "See Other";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  default:
    return "Redirect";
  }
}
} // namespace

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

Client::Client() {
  _keepAlive = HTTP::DEFAULT_KEEP_ALIVE;
  _clientFd = FileDescriptor();
  _remoteAddress = SocketAddress();
  _request = HttpRequest();
  _response = HttpResponse();
  _responseBuffer = std::deque<HttpResponse>();
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize == -1) {
    // handle error: fallback, throw, or use a default
    perror("sysconf");
    pageSize = 4096; // safe default on most systems
  }
  _receiveBuffer = std::vector<char>(
      static_cast<size_t>(pageSize)); // Is about 4KB depending on the system
  _potentialServers = NULL;
  _state = WAITING_FOR_EPOLLIN;
  _lastActivity = time(NULL);
}

Client::Client(const Client &src) { *this = src; }

Client::Client(FileDescriptor socketFd, SocketAddress remoteAddress) {
  _keepAlive = HTTP::DEFAULT_KEEP_ALIVE;
  _clientFd = socketFd;
  _remoteAddress = remoteAddress;
  _request = HttpRequest();
  _request.setRemoteAddress(&_remoteAddress);
  _response = HttpResponse();
  _responseBuffer = std::deque<HttpResponse>();
  long pageSize = sysconf(_SC_PAGESIZE);
  if (pageSize == -1) {
    perror("sysconf");
    pageSize = 4096;
  }
  _receiveBuffer = std::vector<char>(static_cast<size_t>(pageSize));
  _potentialServers = NULL;
  _state = WAITING_FOR_EPOLLIN;
  _lastActivity = time(NULL);
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

Client::~Client() {}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

Client &Client::operator=(const Client &rhs) {
  if (this != &rhs) {
    _clientFd = rhs._clientFd;
    _remoteAddress = rhs._remoteAddress;
    _request = rhs._request;
    // Rebind HttpRequest's remote address pointer to this instance's
    // _remoteAddress
    _request.setRemoteAddress(&_remoteAddress);
    _response = rhs._response;
    _responseBuffer = rhs._responseBuffer;
    _receiveBuffer = rhs._receiveBuffer;
    _holdingBuffer = rhs._holdingBuffer;
    _potentialServers = rhs._potentialServers;
    _state = rhs._state;
    _lastActivity = rhs._lastActivity;
    _keepAlive = rhs._keepAlive;
  }
  return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void Client::handleEvent(epoll_event event) {
  if (event.events & EPOLLIN)
    _handleBuffer();
  if (event.events & EPOLLOUT)
    _handleResponseBuffer();
  if (event.events & EPOLLERR) {
    Logger::error("Client: Error event occurred for client: " +
                      _remoteAddress.getHostString() + ":" +
                      _remoteAddress.getPortString(),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    _state = DISCONNECTED;
  }
  if (event.events & EPOLLHUP) {
    Logger::error("Client: Hang-up event occurred for client: " +
                      _remoteAddress.getHostString() + ":" +
                      _remoteAddress.getPortString(),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    _state = DISCONNECTED;
  }
  updateActivity(); // base last activity time off of when event handled
}

void Client::_handleBuffer() {
  while (true) {
    ssize_t bytesRead =
        recv(_clientFd.getFd(), &_receiveBuffer[0], _receiveBuffer.size(), 0);
    if (bytesRead > 0) {
      _holdingBuffer.insert(_holdingBuffer.end(), _receiveBuffer.begin(),
                            _receiveBuffer.begin() + bytesRead);
    } else if (bytesRead == 0) {
      Logger::warning("Client: " + _remoteAddress.getHostString() + ":" +
                          _remoteAddress.getPortString() + " disconnected",
                      __FILE__, __LINE__, __PRETTY_FUNCTION__);
      _state = DISCONNECTED;
      return;
    } else // No idea if EGAIN or something else due to project restrictions
           // just have to assume its EGAIN and process as normal
    {
      break;
    }
  }
  // Parse the request
  _handleRequest();
}

void Client::_handleRequest() {
  while (!_holdingBuffer.empty()) {
    // Set/refresh current potential servers if not set for the request yet
    if (_request.getPotentialServers() == NULL)
      _request.setPotentialServers(_potentialServers);
    HttpRequest::ParseState parseState =
        _request.parseBuffer(_holdingBuffer, _response);
    switch (parseState) {
    case HttpRequest::PARSING_COMPLETE:
      _routeRequest();
      _responseBuffer.push_back(_response);
      _response.reset();
      _request.reset();
      // Set state to waiting for epollout here as we know we have responses
      // ready
      _state = WAITING_FOR_EPOLLOUT;
      break;
    case HttpRequest::PARSING_ERROR:
      // Any errors here are considered fatal and denote an immediate disconnect
      _responseBuffer.push_back(_response);
      _state = WAITING_FOR_EPOLLOUT;
      return;
    case HttpRequest::PARSING_URI:
    case HttpRequest::PARSING_HEADERS:
    case HttpRequest::PARSING_BODY:
      // Ending on any of these states means we need more data to complete the
      // request
      return;
    default:
      break;
    }
  }
}

void Client::_routeRequest() {
  // Change keep alive setting depending on rules set in location
  _keepAlive = _request.getSelectedLocation()->getKeepAliveValue();

  const HttpURI &uri = _request.getURI();

  // Verify method is allowed by location
  const Location *location = _request.getSelectedLocation();
  Logger::debug(
      "Client: Routing request for " + uri.getResolvedPath() +
          ", location type: " +
          StrUtils::toString(location->getLocationType()) +
          ", cgi value: " + StrUtils::toString(location->getisCgiPathValue()),
      __FILE__, __LINE__, __PRETTY_FUNCTION__);
  if (std::find(location->getAllowedMethods()->begin(),
                location->getAllowedMethods()->end(),
                uri.getMethod()) ==
      location->getAllowedMethods()->end()) // 2. Verify method is allowed
  {
    _response.setResponseDefaultBody(405, "Method Not Allowed", location,
                                     HttpResponse::ERROR);
    std::string allowHeaderValue = "Allow: ";
    const std::vector<std::string> *allowedMethods =
        location->getAllowedMethods();
    for (size_t i = 0; i < allowedMethods->size(); ++i) {
      allowHeaderValue += (*allowedMethods)[i];
      if (i < allowedMethods->size() - 1)
        allowHeaderValue += ", ";
    }
    _response.setHeader(Header(allowHeaderValue));
    return;
  }

  if (location->getLocationType() == Location::REDIRECT) {
    const std::pair<int, std::string> *redirect = location->getRedirect();
    if (redirect) {
      _response.setRedirectResponse(redirect->first,
                                    getRedirectReasonPhrase(redirect->first),
                                    redirect->second, HttpResponse::SUCCESS);
    } else {
      Logger::error("Client: Redirect location missing target", __FILE__,
                    __LINE__, __PRETTY_FUNCTION__);
      _response.setResponseDefaultBody(500, "Invalid redirect configuration",
                                       location, HttpResponse::ERROR);
    }
    return;
  }

  if (location->getLocationType() == Location::CGI) {
    Logger::log(Logger::INFO,
                "Client: CGI handler called for " + uri.getResolvedPath());
    CgiHandler cgiHandler;
    CgiHandler::ExecutionResult result = cgiHandler.execute(
        _request, _response, _request.getSelectedServer(), location);
    Logger::debug("Client: CGI handler completed with result code " +
                      StrUtils::toString(result),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    return;
  }

  IMethodHandler *handler =
      MethodHandlerFactory::createHandler(uri.getMethod());
  if (handler) {
    Logger::debug("Client: Created handler for method: " + uri.getMethod(),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    Logger::debug("Client: For path: " + uri.getResolvedPath(), __FILE__,
                  __LINE__, __PRETTY_FUNCTION__);
    handler->handleRequest(_request, _response, *location);
    delete handler;
  } else {
    Logger::error("Client: Failed to create handler for method: " +
                      uri.getMethod(),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    _response.setResponseDefaultBody(
        403, "Method Not Implemented: " + uri.getMethod(), location,
        HttpResponse::ERROR);
  }
}

// Write up to 4096 worth of response to the client each time this is called
// Highlevel consideration is to flush the response buffer so EPOLLOUT takes
// priority over EPOLLIN
void Client::_handleResponseBuffer() {
  Logger::debug("Client: Handling response buffer for client: " +
                    _remoteAddress.getHostString() + ":" +
                    _remoteAddress.getPortString(),
                __FILE__, __LINE__, __PRETTY_FUNCTION__);
  errno = 0;
  ssize_t totalBytesSent = 0;
  // SafeGuard should never occur
  if (_responseBuffer.empty()) {
    Logger::error("Client: Response buffer is empty while handling response "
                  "buffer for client: " +
                      _remoteAddress.getHostString() + ":" +
                      _remoteAddress.getPortString(),
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
    _state = WAITING_FOR_EPOLLIN;
    return;
  }
  HttpResponse &response = _responseBuffer.front();
  while (totalBytesSent < HTTP::DEFAULT_SEND_SIZE && !_responseBuffer.empty()) {
    response.sendResponse(_clientFd, totalBytesSent);
    switch (response.getSendingState()) {
    case HttpResponse::RESPONSE_SENDING_COMPLETE: {
      switch (response.getResponseType()) {
      case HttpResponse::SUCCESS:
      case HttpResponse::ERROR:
        _responseBuffer.pop_front(); // Clear response from buffer when its done
        if (!_responseBuffer.empty()) // If the response buffer is not empty set
                                      // the next response to send
        {
          response = _responseBuffer.front();
          _state = WAITING_FOR_EPOLLOUT;
        } else
          _state =
              WAITING_FOR_EPOLLIN; // else ready to continue processing data
        if (!_keepAlive)
          _state = DISCONNECTED; // If keep alive is false however then we
                                 // disconnect the client
        break;
      case HttpResponse::FATAL_ERROR:
        _state = DISCONNECTED;
        return;
      }
      break;
    }
    case HttpResponse::
        RESPONSE_SENDING_ERROR: // Fatal error encountered sending the response
                                // immediately disconnect the client
      Logger::error("Client: Fatal error encountered while sending response "
                    "for client: " +
                        _remoteAddress.getHostString() + ":" +
                        _remoteAddress.getPortString() + ": " + strerror(errno),
                    __FILE__, __LINE__, __PRETTY_FUNCTION__);
      _state = DISCONNECTED;
      return;
    case HttpResponse::RESPONSE_SENDING_MESSAGE:
    case HttpResponse::RESPONSE_SENDING_BODY: {
      // In the middle of sending the response only ends here if 4096 bytes
      // where reached
      _state = WAITING_FOR_EPOLLOUT;
      break;
    }
    default:
      break;
    }
  }
}

/*
** --------------------------------- ACCESSORS --------------------------------
*/

Client::ClientState Client::getCurrentState() const { return _state; }

void Client::setState(ClientState newState) { _state = newState; }

void Client::updateActivity() { _lastActivity = time(NULL); }

int Client::getSocketFd() const { return _clientFd.getFd(); }

const SocketAddress &Client::getRemoteAddr() const { return _remoteAddress; }

const std::vector<Server> &Client::getPotentialServers() const {
  return *_potentialServers;
}

void Client::setPotentialServers(const std::vector<Server> &potentialServers) {
  _potentialServers = &potentialServers;
}

bool Client::isTimedOut() const {
  time_t currentTime = time(NULL);
  return (currentTime - _lastActivity) > 30; // 30 second timeout
}
