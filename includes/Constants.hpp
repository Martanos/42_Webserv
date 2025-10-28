#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "Logger.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <unistd.h>

// System constants all in one place
// HTTP Constants
namespace HTTP
{
const std::string SINGLETON_HEADERS[] = {"content-type",  "location",		  "user-agent", "referer",
										 "authorization", "content-location", "date",		"etag",
										 "expires",		  "last-modified",	  "server"};
static const std::string SUPPORTED_METHODS[] = {"GET", "POST", "DELETE"};
static const std::string HTTP_VERSION = "HTTP/1.1";
static const std::string TEMP_FILE_TEMPLATE = "/tmp/webserv-";
static const size_t MAX_URI_SIZE = 16384;		 // 16KB
static const size_t MAX_HEADER_SIZE = 16384;	 // 16KB
static const size_t MAX_BODY_SIZE = 1048576; // 1MB
const int DEFAULT_TIMEOUT_SECONDS = 30;		 // 30 second timeout

// HTTP Status Codes
const int STATUS_CONTINUE = 100;
const int STATUS_SWITCHING_PROTOCOLS = 101;
const int STATUS_PROCESSING = 102;
const int STATUS_EARLY_HINTS = 103;
const int STATUS_OK = 200;
const int STATUS_CREATED = 201;
const int STATUS_ACCEPTED = 202;
const int STATUS_NON_AUTHORITATIVE_INFORMATION = 203;
const int STATUS_NO_CONTENT = 204;
const int STATUS_RESET_CONTENT = 205;
const int STATUS_PARTIAL_CONTENT = 206;
const int STATUS_MULTI_STATUS = 207;
const int STATUS_ALREADY_REPORTED = 208;
const int STATUS_IM_USED = 226;
const int STATUS_MULTIPLE_CHOICES = 300;
const int STATUS_MOVED_PERMANENTLY = 301;
const int STATUS_FOUND = 302;
const int STATUS_SEE_OTHER = 303;
const int STATUS_NOT_MODIFIED = 304;
const int STATUS_USE_PROXY = 305;
const int STATUS_TEMPORARY_REDIRECT = 307;
const int STATUS_PERMANENT_REDIRECT = 308;
const int STATUS_BAD_REQUEST = 400;
const int STATUS_UNAUTHORIZED = 401;
const int STATUS_FORBIDDEN = 403;
const int STATUS_NOT_FOUND = 404;
const int STATUS_METHOD_NOT_ALLOWED = 405;
const int STATUS_REQUEST_ENTITY_TOO_LARGE = 413;
const int STATUS_URI_TOO_LONG = 414;
const int STATUS_UNSUPPORTED_MEDIA_TYPE = 415;
const int STATUS_RANGE_NOT_SATISFIABLE = 416;
const int STATUS_EXPECTATION_FAILED = 417;
const int STATUS_IM_A_TEAPOT = 418;
const int STATUS_MISDIRECTED_REQUEST = 421;
const int STATUS_UNPROCESSABLE_ENTITY = 422;
const int STATUS_LOCKED = 423;
const int STATUS_FAILED_DEPENDENCY = 424;
const int STATUS_TOO_MANY_REQUESTS = 429;
const int STATUS_REQUEST_HEADER_FIELDS_TOO_LARGE = 431;
const int STATUS_UNAVAILABLE_FOR_LEGAL_REASONS = 451;
const int STATUS_CLIENT_CLOSED_REQUEST = 499;
const int STATUS_INTERNAL_SERVER_ERROR = 500;
const int STATUS_NOT_IMPLEMENTED = 501;
const int STATUS_SERVICE_UNAVAILABLE = 503;
} // namespace HTTP

// Server Constants
namespace SERVER
{
const char *const DEFAULT_HOST = "0.0.0.0";
const unsigned short DEFAULT_PORT = 80;
const char *const DEFAULT_ROOT = "www/";
const char *const DEFAULT_INDEX = "index.html";
const bool DEFAULT_AUTOINDEX = false;
const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 1048576; // 1MB
const bool DEFAULT_KEEP_ALIVE = true;

const char *const SERVER_VERSION = "42_Webserv/1.0";
} // namespace SERVER

#endif /* CONSTANTS_HPP */
