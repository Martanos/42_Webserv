#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <sys/types.h>
#include <unistd.h>
#include <cstddef>

// System constants all in one place
// HTTP Constants
namespace HTTP
{
	const size_t MAX_HEADER_SIZE = sysconf(_SC_PAGE_SIZE);		 // 8KB max for headers
	const size_t MAX_REQUEST_LINE_SIZE = sysconf(_SC_PAGE_SIZE); // 2KB max for request line
	const size_t DEFAULT_BUFFER_SIZE = sysconf(_SC_PAGE_SIZE);	 // 4KB default read buffer
	const size_t CHUNK_READ_SIZE = sysconf(_SC_PAGE_SIZE);		 // 8KB chunk size for file operations
	const int DEFAULT_TIMEOUT_SECONDS = 30;						 // 30 second timeout
	const int MAX_CONNECTIONS = sysconf(_SC_SOMAXCONN);			 // Maximum concurrent connections

	// HTTP Status Codes
	const int STATUS_OK = 200;
	const int STATUS_CREATED = 201;
	const int STATUS_NO_CONTENT = 204;
	const int STATUS_MOVED_PERMANENTLY = 301;
	const int STATUS_FOUND = 302;
	const int STATUS_BAD_REQUEST = 400;
	const int STATUS_UNAUTHORIZED = 401;
	const int STATUS_FORBIDDEN = 403;
	const int STATUS_NOT_FOUND = 404;
	const int STATUS_METHOD_NOT_ALLOWED = 405;
	const int STATUS_REQUEST_ENTITY_TOO_LARGE = 413;
	const int STATUS_INTERNAL_SERVER_ERROR = 500;
	const int STATUS_NOT_IMPLEMENTED = 501;
	const int STATUS_SERVICE_UNAVAILABLE = 503;
}

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
}

// File System Constants
namespace FS
{
	const size_t MAX_PATH_LENGTH = 4096;
	const size_t MAX_FILENAME_LENGTH = 255;
	const char PATH_SEPARATOR = '/';
}

// Network Constants
namespace NET
{
	const int SOMAXCONN_VALUE = 128;
	const size_t MAX_EPOLL_EVENTS = 1024;
}

#endif /* CONSTANTS_HPP */
