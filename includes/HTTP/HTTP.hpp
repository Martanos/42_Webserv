#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include "../../includes/Global/Logger.hpp"
#include "../../includes/Wrapper/FileDescriptor.hpp"
#include <cstddef>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// System constants all in one place
// HTTP Constants
namespace HTTP
{
const char *const SINGLETON_HEADERS[] = {
	"host",			 "content-length",		"content-type", "content-location", "date",		   "etag",
	"expires",		 "last-modified",		"location",		"server",			"user-agent",  "referer",
	"authorization", "proxy-authorization", "expect",		"upgrade",			"retry-after", "content-range"};
static const std::string HTTP_VERSION = "HTTP/1.1";
static const std::string TEMP_FILE_TEMPLATE = "/tmp/webserv-";
static const ssize_t MAX_URI_LINE_SIZE = 16384;		 // 16KB
static const ssize_t MAX_HEADERS_LINE_SIZE = 16384;	 // 16KB
static const ssize_t MAX_HEADERS_SIZE = 32768;		 // 32KB
static const ssize_t MAX_BODY_BUFFER_SIZE = 1048576; // 1MB
static const ssize_t MAX_BODY_SIZE = 10485760;		 // 10MB
static const char *const CRLF = "\r\n";				 // CRLF
const int DEFAULT_TIMEOUT_SECONDS = 30;				 // 30 second timeout
static const std::string DEFAULT_HOST = "0.0.0.0";
static const unsigned short DEFAULT_PORT = 80;
static const bool DEFAULT_AUTOINDEX = false;
static const bool DEFAULT_KEEP_ALIVE = true;

inline bool isSupportedMethod(const std::string &method)
{
	return method == "GET" || method == "POST" || method == "DELETE";
}


} // namespace HTTP

#endif /* HTTP_HPP */
