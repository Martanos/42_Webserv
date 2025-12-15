# Webserv Copilot Instructions

## Project Overview

This is a **42 School webserv project** - an HTTP/1.1 compliant static file server written in **C++98**. The server uses an event-driven, non-blocking architecture with `epoll()` for I/O multiplexing on Linux.

## Build & Run

```bash
make              # Build with default logging (WARNING+)
make debug        # Build with full DEBUG logging (LOG_MIN_LEVEL=0)
make re           # Clean rebuild
./webserv <config_file>   # Run with config (e.g., webserv.conf, configs/debug.conf)
```

**Testing:**
```bash
./Test_scripts/run_full_test.sh    # Full integration test suite
./Project_resources/ubuntu_tester  # 42's official tester
```

## Architecture

### Core Flow
1. **Config** (`srcs/Config/`) → Parse NGINX-like config into `Server` objects
2. **ServerMap** → Maps `host:port` to servers via `TrieTree<Location>` for fast path matching
3. **ServerManager** (`srcs/Core/`) → Main epoll event loop
4. **Client** (`srcs/Core/Client.cpp`) → State machine: `WAITING_FOR_EPOLLIN` → `WAITING_FOR_EPOLLOUT` → `DISCONNECTED`
5. **MethodHandlers** (`srcs/MethodHandlers/`) → GET/POST/PUT/DELETE via factory pattern

### Key Components
| Directory | Purpose |
|-----------|---------|
| `Core/` | Event loop, client state machine, epoll management |
| `Http/` | Request parsing (HttpURI, HttpHeaders, HttpBody), response generation |
| `Config/` | Tokenizer → Parser → AST → ConfigTranslator → Server objects |
| `MethodHandlers/` | IMethodHandler interface + GET/POST/PUT/DELETE implementations |
| `Cgi/` | Fork/exec CGI scripts (CgiHandler, CgiExecutor, CgiEnv, CgiResponse) |
| `Containers/` | Custom `TrieTree` for O(k) path matching, `RingBuffer` for I/O |
| `Wrappers/` | RAII wrappers (FileDescriptor, ListeningSocket, SocketAddress) |
| `Global/` | Singletons/utilities: Logger, MimeTypeResolver, PerformanceMonitor |
| `Utils/` | Header-only utilities: StrUtils, FileUtils, IPAddressParser |

---

## Detailed File Reference

### Entry Point

#### `srcs/main.cpp`
- Initializes `Logger` session for file/console logging
- Parses command-line argument for config file path
- Creates `ConfigParser` to parse config file into AST
- Uses `ConfigTranslator` to convert AST → `ServerMap` with `Server` objects
- Instantiates `ServerManager` with the `ServerMap`
- Calls `ServerManager::run()` to start the event loop

---

### Core Module (`srcs/Core/`)

#### `ServerManager.cpp`
- **Main event loop** using epoll for non-blocking I/O multiplexing
- `run()` → Infinite loop calling `epoll_wait()` and processing events
- **Ignores SIGPIPE** at startup via `signal(SIGPIPE, SIG_IGN)` to prevent crashes when writing to broken pipes (e.g., CGI child exits early)
- Handles new connections on listening sockets via `_acceptNewConnection()`
- Dispatches read/write events to appropriate `Client` objects
- Manages client lifecycle (creation, event handling, disconnection)
- Uses `EpollManager` to add/modify/remove file descriptors from epoll

#### `Client.cpp`
- **Per-client state machine** managing HTTP request/response cycle
- States: `WAITING_FOR_EPOLLIN` → `WAITING_FOR_EPOLLOUT` → `DISCONNECTED`
- `readIntoBuffer()` → Reads data from socket into `RingBuffer`
- `processRequest()` → Parses request, routes to appropriate method handler
- `writeFromBuffer()` → Sends response data from buffer to socket
- Owns `HttpRequest` and `HttpResponse` objects for the connection
- Handles keep-alive vs close connection logic

#### `EpollManager.cpp`
- **RAII wrapper** around Linux epoll API
- `addFd(fd, events)` → Register fd with epoll (EPOLLIN, EPOLLOUT, etc.)
- `modifyFd(fd, events)` → Change monitored events for fd
- `removeFd(fd)` → Unregister fd from epoll
- `wait(events, maxEvents, timeout)` → Blocks until events ready
- Sets EPOLLRDHUP for detecting client disconnections

---

### HTTP Module (`srcs/Http/`)

#### `HttpRequest.cpp`
- **Request parsing state machine** with states: `PARSING_URI`, `PARSING_HEADERS`, `PARSING_BODY`, `PARSING_COMPLETE`, `PARSING_ERROR`
- `parse(ringBuffer)` → Incrementally parses request from buffer
- `identifyServer()` → Matches `Host` header to virtual server in `ServerMap`
- `identifyLocation()` → Finds matching `Location` using TrieTree prefix matching
- Handles chunked transfer encoding and content-length bodies
- Tracks internal redirects (max 5) for CGI `Location:` headers

#### `HttpResponse.cpp`
- **Builds HTTP responses** with status line, headers, body
- `setResponseDefaultBody(statusCode, message, location, type)` → Uses custom error pages if configured
- `setResponseCustomBody(statusCode, reason, body, contentType, type)` → Custom content
- `setFileBody(filePath)` → Streams file content with correct MIME type
- `toString()` → Serializes response for socket transmission
- Generates `Date`, `Content-Length`, `Content-Type`, `Connection` headers

#### `HttpURI.cpp`
- **URI parsing and path resolution**
- `parse(rawUri)` → Extracts scheme, host, port, path, query string, fragment
- `decode(encoded)` → Handles percent-encoding (%20 → space, etc.)
- `resolve(location, server)` → Computes filesystem path from root + location + path
- Validates path safety (no `..`, no null bytes, within root directory)

#### `HttpHeaders.cpp`
- **HTTP header parsing** from raw buffer
- State machine for header lines (handles CRLF, line folding)
- `getHeader(name)` → Case-insensitive header lookup
- Stores headers in `std::map<std::string, Header>`
- Validates header syntax per HTTP/1.1 spec

#### `HttpBody.cpp`
- **Request body parsing**
- Handles `Content-Length` based bodies
- Handles chunked transfer encoding (parses chunk sizes, trailers)
- Enforces `client_max_body_size` limit (returns 413 if exceeded)
- `getBody()` → Returns accumulated body data

#### `Header.cpp`
- **Single header line parsing**
- Extracts directive (header name) and values
- Handles multi-value headers (comma-separated)

---

### Config Module (`srcs/Config/`)

#### `ConfigTokeniser.cpp`
- **Lexer** for config file format
- Tokenizes input into: `WORD`, `LBRACE`, `RBRACE`, `SEMICOLON`, `NEWLINE`
- Handles comments (lines starting with `#`)
- Skips whitespace, preserves token positions for error reporting

#### `ConfigParser.cpp`
- **Recursive descent parser** building AST from tokens
- Parses `server { }` and `location { }` blocks
- Validates directive syntax and nesting rules
- Produces tree of config nodes for translation

#### `ConfigTranslator.cpp`
- **AST → Server objects transformation**
- Creates `Server` instances with `Location` objects
- Validates directive combinations and required fields
- Builds `ServerMap` mapping `host:port` → server list
- Handles virtual hosting (multiple servers on same port)

#### `Server.cpp`
- **Server configuration storage**
- Contains `TrieTree<Location>` for location matching
- `findLocation(path)` → Returns matching location or NULL
- `findLongestPrefix(path)` → Prefix-based location matching
- Stores server-level directives (root, index, error_pages, etc.)

#### `Location.cpp`
- **Location block configuration**
- Stores location-specific directives (allowed_methods, cgi_path, upload_path)
- `getLocationType()` → Returns `STATIC`, `CGI`, `UPLOAD`, or `REDIRECT`
- Inherits from `Directives` for common directive handling

#### `Directives.cpp`
- **Base class** for Server and Location directive storage
- Provides `has*()` and `get*()` accessors for all directives
- Handles directive inheritance (location inherits from server)

#### `ServerMap.cpp`
- **Maps listening sockets to server vectors**
- Key: `ListeningSocket` (host:port combination)
- Value: `std::vector<Server*>` for virtual hosting
- `getServers(socket)` → Returns servers for that socket
- Used by Client to identify target server from Host header

#### `ConfigFileReader.cpp`
- **File reading utility** for config loading
- Reads entire config file into string
- Handles file not found and permission errors

---

### CGI Module (`srcs/Cgi/`)

#### `CgiHandler.cpp`
- **Orchestrator** for CGI execution pipeline
- `execute(scriptPath, request, response, location, server)` → Main entry point
- Coordinates: environment setup → execution → response parsing
- Handles errors and sets appropriate HTTP status codes
- Returns `ExecutionResult` enum indicating success/failure type

#### `CgiEnv.cpp`
- **Builds CGI environment variables**
- Sets standard CGI variables: `REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_TYPE`, `CONTENT_LENGTH`, `SCRIPT_NAME`, `PATH_INFO`, `SERVER_NAME`, `SERVER_PORT`, etc.
- Converts HTTP headers to `HTTP_*` environment variables
- `getEnvArray()` → Returns `char**` for `execve()`

#### `CgiExecutor.cpp`
- **Fork/exec with pipe management**
- Creates pipes for stdin (request body), stdout (response), stderr (errors)
- Forks child process, sets up pipes, calls `execve()`
- Parent process: writes request body to stdin, reads response from stdout
- Implements timeout handling (default 30s) with SIGALRM
- Handles EINTR from signal interruption

#### `CgiResponse.cpp`
- **Parses CGI script output**
- Separates headers from body (blank line delimiter)
- Handles `Status:` pseudo-header for custom status codes
- Detects NPH (Non-Parsed Header) mode
- Detects internal redirects (`Location:` header with local path)
- Populates `HttpResponse` with parsed data

---

### Method Handlers (`srcs/MethodHandlers/`)

#### `MethodHandlerFactory.cpp`
- **Factory pattern** for method handler creation
- `getHandler(method)` → Returns appropriate handler for HTTP method
- Returns NULL for unsupported methods (triggers 405 response)

#### `GetMethodHandler.cpp`
- **Handles GET requests**
- `handleRequest(request, response, location)` → Main entry point
- Checks if CGI location → delegates to `CgiHandler`
- Serves static files with correct MIME type
- Generates directory listings if `autoindex on`
- Handles index file lookup (index.html, etc.)

#### `PostMethodHandler.cpp`
- **Handles POST requests**
- Checks if CGI location → delegates to `CgiHandler`
- Handles file uploads to configured `upload_path`
- Generates unique filenames for uploaded files
- Returns 201 Created on successful upload

#### `PutMethodHandler.cpp`
- **Handles PUT requests**
- Checks if CGI location → delegates to `CgiHandler`
- Creates or replaces files at requested path
- Creates parent directories if needed
- Returns 201 Created (new file) or 204 No Content (replaced)

#### `DeleteMethodHandler.cpp`
- **Handles DELETE requests**
- Checks if CGI location → delegates to `CgiHandler`
- Deletes files at requested path
- Validates file type (only regular files allowed)
- Returns 204 No Content on successful deletion

---

### Containers (`srcs/Containers/`, `includes/Containers/`)

#### `RingBuffer.cpp` / `RingBuffer.hpp`
- **Circular buffer** for efficient I/O operations
- Fixed-size buffer that wraps around
- `writeBuffer(data, len)` → Append data to buffer
- `readBuffer(dest, len)` → Read and consume data
- `peekBuffer(dest, len)` → Read without consuming
- Used for socket read/write buffering

#### `TrieTree.hpp` (Header-only template)
- **Prefix tree** for O(k) path matching
- `insert(key, value)` → Add path → location mapping
- `find(key)` → Exact match lookup
- `findLongestPrefix(key)` → Prefix matching for location routing
- Stores keys exactly as provided (no normalization)
- `_isValidKey()` → Validates keys (rejects null bytes)

#### `TrieNode.hpp` (Header-only template)
- **Node structure** for TrieTree
- Contains `std::map<char, TrieNode*>` for children
- Stores optional value at terminal nodes

---

### Wrappers (`srcs/Wrappers/`)

#### `FileDescriptor.cpp`
- **RAII wrapper** for file descriptors
- Reference-counted (multiple FileDescriptor objects can share same fd)
- Auto-closes fd when last reference destroyed
- `setNonBlocking()` → Sets O_NONBLOCK via fcntl
- `setReuseAddr()` → Sets SO_REUSEADDR for socket
- Static factories: `createSocket()`, `createFromAccept()`

#### `ListeningSocket.cpp`
- **Encapsulates server listening socket**
- Contains `FileDescriptor` and `SocketAddress`
- `bind()` → Binds socket to address
- `listen()` → Puts socket in listening mode
- `accept(remoteAddr, clientFd)` → Accepts new connection

#### `SocketAddress.cpp`
- **Encapsulates IPv4/IPv6 socket addresses**
- Parses host:port strings into `sockaddr_storage`
- Supports IPv4, IPv6, and hostname resolution via `getaddrinfo()`
- `getHost()` / `getPort()` → Returns cached string values
- `getSockAddr()` → Returns raw `sockaddr*` for socket calls

#### `FileManager.cpp`
- **File operations utility**
- `copyFile(src, dest, overwrite)` → Copies files (handles cross-mount)
- `moveFile(src, dest, overwrite)` → Moves files (uses rename or copy+delete)
- `deleteFile(path)` → Removes file
- `createDirectory(path)` → Creates directory with parents

---

### Global Utilities (`srcs/Global/`)

#### `Logger.cpp`
- **Centralized logging** with file and console output
- Levels: `DEBUG(0)`, `INFO(1)`, `WARNING(2)`, `ERROR(3)`, `CRITICAL(4)`
- `initializeSession(logDir)` → Creates session log file
- `log(level, message, file, line, function)` → Log with source context
- Compile-time filtering via `LOG_MIN_LEVEL` macro
- Color-coded console output

#### `MimeTypeResolver.cpp`
- **MIME type detection**
- Loads `/etc/mime.types` for extension-based lookup
- Magic byte detection for binary files (PNG, JPEG, GIF, etc.)
- `getMimeType(filePath)` → Returns Content-Type string
- Fallback: `application/octet-stream`

#### `PerformanceMonitor.cpp`
- **Singleton** for performance metrics
- Tracks request counts, response times, memory usage
- `Timer` nested class for operation timing
- `logStats()` → Outputs performance summary
- Used for debugging and optimization

---

### Header-Only Utilities (`includes/Utils/`)

#### `StrUtils.hpp`
- **String manipulation functions**
- `toLowerCase()` / `toUpperCase()` → Case conversion
- `trimSpaces()` → Whitespace trimming
- `splitString(str, delimiter)` → String tokenization
- `toString<T>()` / `fromString<T>()` → Type conversion (C++98 compatible)
- Character validation: `isControlCharacter()`, `isValidHeaderChar()`, etc.

#### `FileUtils.hpp`
- **File system utilities**
- `isFileReadable()` / `isFileWritable()` / `isFileExecutable()` → Access checks
- `fileExists()` → Existence check
- `normalizePath()` → Resolves symlinks via `realpath()`
- `getFileExtension()` / `getFileName()` → Path extraction
- `inRoot()` → Validates path is within allowed directory

#### `IPAddressParser.hpp`
- **IP address parsing**
- `parseIPv4(str, result)` → Parses dotted-decimal to network order
- `parseIPv6(str, result)` → Basic IPv6 parsing
- Used by `SocketAddress` for address resolution

## Coding Conventions

### C++98 Strict Compliance
- **No** `auto`, range-based for, lambdas, `nullptr`, smart pointers
- Use `StrUtils::toString<T>()` instead of `std::to_string()`
- Use `std::vector<T>::iterator` explicitly

### Class Structure
Follow Orthodox Canonical Form for all classes:
```cpp
ClassName();                              // Default constructor
ClassName(const ClassName &src);          // Copy constructor
ClassName &operator=(const ClassName &rhs); // Assignment operator
~ClassName();                             // Destructor
```

### Logging
Use the centralized `Logger` class with file/line context:
```cpp
Logger::log(Logger::DEBUG, "message", __FILE__, __LINE__, __PRETTY_FUNCTION__);
Logger::log(Logger::INFO, "message");  // Simple form for less critical logs
// Levels: DEBUG(0), INFO(1), WARNING(2), ERROR(3), CRITICAL(4)
```

### Error Responses
Use `HttpResponse::setResponseDefaultBody()` with location for custom error pages:
```cpp
response.setResponseDefaultBody(404, "Not Found", &location, HttpResponse::ERROR);
```

### Directive Pattern
`Server` and `Location` both inherit from `Directives`. Check directive presence before access:
```cpp
if (location.hasRootPathDirective())
    const std::string *root = location.getRootPath();
```

## Key Data Structures

### TrieTree (Path Matching)
Used for O(k) location matching where k = path length:
```cpp
TrieTree<Location> _locations;  // In Server class
bool hasLongestPrefixLocation(const std::string &path);  // Prefix match
const Location *getLocation(const std::string &path);    // Exact match
```

### HttpRequest Parse States
```cpp
enum ParseState { PARSING_URI, PARSING_HEADERS, PARSING_BODY, PARSING_COMPLETE, PARSING_ERROR };
```

### Client States
```cpp
enum ClientState { WAITING_FOR_EPOLLIN, WAITING_FOR_EPOLLOUT, DISCONNECTED };
```

## CGI Handling

CGI execution for dynamic content (PHP, Python scripts in `www/cgi-bin/` or locations with `cgi_path true`).

### CGI Flow
```
GetMethodHandler::_serveFile() / PostMethodHandler::handleRequest()
    └── CgiHandler::execute(scriptPath, request, response, &location, server)
            ├── CgiEnv::_transposeData() → Build CGI environment variables
            ├── CgiExecutor::execute()   → Fork/exec with pipes, timeout handling
            └── CgiResponse::populateHttpResponse() → Parse script output into HttpResponse
```

### Key Classes
| Class | Responsibility |
|-------|----------------|
| `CgiHandler` | Orchestrator - coordinates env setup, execution, response parsing |
| `CgiEnv` | Builds CGI environment variables (REQUEST_METHOD, QUERY_STRING, etc.) |
| `CgiExecutor` | Fork/exec, pipe management (stdin/stdout/stderr), timeout (default 30s), child exit detection |
| `CgiResponse` | Parses CGI output (headers + body), handles NPH mode, populates HttpResponse (body before headers to preserve CGI headers) |

### CGI Implementation Notes

#### Signal Handling
- **SIGPIPE ignored** in `ServerManager::run()` via `signal(SIGPIPE, SIG_IGN)` - prevents server crash when CGI child exits unexpectedly while parent is writing to stdin pipe

#### Child Process Detection
- `CgiExecutor::_communicateWithChild()` uses `waitpid(WNOHANG)` to detect early child termination during pipe I/O
- Prevents indefinite blocking when child process exits before reading all stdin data

#### CGI Response Processing
- `CgiResponse::populateHttpResponse()` sets body **before** headers - ensures CGI-provided headers (Status, Content-Type, Location, Content-Disposition) override defaults set by `setBody()`

#### Header Formatting
- `HttpResponse::sendResponse()` uses `operator<<` to format headers with parameters (e.g., `Content-Disposition: attachment; filename="file.txt"`)
- `Header::operator<<` properly quotes parameter values and uses `getDirective()` for the header name

#### File Upload Body Handling
- `HttpBody` uses `fsync()` after writing multipart body to temp file - ensures data is flushed to disk before CGI reads it
- CGI scripts receive body via stdin pipe from temp file

### CgiHandler::execute() Signature
```cpp
ExecutionResult execute(const std::string &scriptPath, const HttpRequest &request,
                        HttpResponse &response, const Location *location,
                        const Server *server = NULL);
```

### ExecutionResult Codes
```cpp
enum ExecutionResult {
    SUCCESS = 0,
    ERROR_INVALID_SCRIPT_PATH = 1,
    ERROR_SCRIPT_NOT_FOUND = 2,      // → 404
    ERROR_EXECUTION_FAILED = 3,      // → 500
    ERROR_RESPONSE_PARSING_FAILED = 4, // → 500
    ERROR_TIMEOUT = 5,               // → 504
    ERROR_INTERNAL_ERROR = 6         // → 500
};
```

### Internal Redirects
CGI scripts can trigger internal redirects by returning a `Location:` header with:
- Path starting with `/` (absolute local path)
- No `://` scheme
- Status 200 (not 3xx)

Tracked via `MAX_INTERNAL_REDIRECTS = 5` in HttpRequest.

### HTTP Redirects from CGI
CGI scripts can issue HTTP redirects using the `Status:` pseudo-header:
```php
header("Status: 303 See Other");
header("Location: /new-page.php?msg=success");
echo "";  // Empty body for redirect
exit;
```

### CGI Script Requirements
PHP/Python CGI scripts must:
1. **Parse QUERY_STRING manually** - `$_GET` and similar are not auto-populated:
   ```php
   parse_str($_SERVER['QUERY_STRING'], $_GET);
   ```
2. **Read stdin with CONTENT_LENGTH** - Don't rely on EOF for POST body:
   ```php
   $contentLength = isset($_SERVER['CONTENT_LENGTH']) ? intval($_SERVER['CONTENT_LENGTH']) : 0;
   $input = '';
   while ($contentLength > 0) {
       $chunk = fread(STDIN, min(8192, $contentLength));
       if ($chunk === false) break;
       $input .= $chunk;
       $contentLength -= strlen($chunk);
   }
   ```
3. **Output proper CGI headers** - At minimum `Content-Type:`, separated from body by blank line

## Config File Format

NGINX-like syntax. See `sample.conf` for full documentation with validation rules.

### Server Block Directives

| Directive | Format | Required | Notes |
|-----------|--------|----------|-------|
| `listen` | `port` or `host:port` | Yes (≥1) | Port range 1-65535. First server per host:port is default |
| `server_name` | `name1 name2 ...` | No | Space-separated hostnames for virtual hosting |
| `root` | `/absolute/path` | Yes* | Base directory for static files |
| `index` | `file1 file2 ...` | No | Default: `index.html`. Checked in order |
| `autoindex` | `on` \| `off` | No | Default: `off`. Directory listing when no index found |
| `client_max_body_size` | `number[K|M|G]` | No | Default: `1M`. Returns 413 if exceeded |
| `status_page` | `code /path` | No | Custom error pages (codes 400-599) |
| `keep_alive` | `keepalive` \| `close` | No | HTTP/1.1 defaults to keepalive |

### Location Block Directives

| Directive | Format | Required | Notes |
|-----------|--------|----------|-------|
| `allowed_methods` | `GET POST DELETE PUT` | Yes | Space-separated. Returns 405 for disallowed |
| `redirect` | `301\|302 URL` | No | Mutually exclusive with `cgi_path` |
| `cgi_path` | `true` \| `false` | No | Marks location for CGI execution |
| `upload_path` | `/absolute/path` | No | Directory for file uploads (must be writable) |

### Configuration Rules
1. At least one `server` block required
2. Location blocks only inside server blocks
3. Directives end with semicolon (`;`)
4. Longest matching location path wins
5. Location directives override server directives

### Example
```nginx
server {
    listen 8080;
    server_name example.local;
    root /var/www/html;
    index index.html index.htm;
    client_max_body_size 10M;
    autoindex off;

    status_page 404 /error/404.html;
    status_page 500 /error/500.html;

    location / {
        allowed_methods GET;
    }

    location /upload/ {
        allowed_methods POST DELETE;
        upload_path /var/www/uploads;
        client_max_body_size 100M;
    }

    location /cgi-bin/ {
        allowed_methods GET POST;
        cgi_path true;
    }
}
```

## Known Bugs / Workarounds

### CGI Issues (FIXED)

#### 1. ~~CGI File Upload Hanging Indefinitely~~ (FIXED)
**Files:** `srcs/Core/ServerManager.cpp`, `srcs/Cgi/CgiExecutor.cpp`, `srcs/Http/HttpBody.cpp`

**Was:** File uploads via CGI would hang indefinitely. The server would fork the CGI process, but the process would never complete.

**Causes & Fixes:**
1. **SIGPIPE crash** - When CGI child exited early, parent writing to stdin pipe received SIGPIPE and crashed. Fixed by adding `signal(SIGPIPE, SIG_IGN)` in `ServerManager::run()`.
2. **No child exit detection** - Parent blocked on write() even after child died. Fixed by checking `waitpid(WNOHANG)` in `CgiExecutor::_communicateWithChild()`.
3. **Temp file not synced** - CGI script read incomplete data from temp file. Fixed by adding `fsync()` in `HttpBody` after writing multipart data.

#### 2. ~~CGI Headers Overwritten by setBody()~~ (FIXED)
**File:** `srcs/Cgi/CgiResponse.cpp`

**Was:** CGI-provided headers like `Content-Type`, `Status`, `Content-Disposition` were being overwritten when `setBody()` was called, because `setBody()` sets default Content-Type.

**Fix:** Reordered `populateHttpResponse()` to call `setBody()` before `setHeader()` for CGI headers, ensuring CGI headers take precedence.

#### 3. ~~Header Parameters Not Included in Response~~ (FIXED)
**Files:** `srcs/Http/HttpResponse.cpp`, `srcs/Http/Header.cpp`

**Was:** Header parameters like `filename` in `Content-Disposition: attachment; filename="file.txt"` were being stripped from responses.

**Fixes:**
1. `HttpResponse::sendResponse()` now uses `operator<<` instead of `getRawHeader()` to format headers
2. `Header::operator<<` fixed to use `getDirective()` for header name and properly quote parameter values

#### 4. ~~$_GET Not Populated in PHP CGI~~ (FIXED)
**File:** `www/html/file_upload.php` (or similar CGI scripts)

**Was:** PHP scripts running as CGI don't automatically populate `$_GET` from the query string.

**Fix:** CGI scripts must manually parse QUERY_STRING:
```php
parse_str($_SERVER['QUERY_STRING'], $_GET);
```

### Path Resolution Issues

#### 1. ~~Location Prefix Matching Ignores Trailing Slashes~~ (FIXED)
**File:** `includes/Containers/TrieTree.hpp`

**Was:** The `_normalizePath()` function stripped trailing slashes from location paths before insertion.

**Fix:** Removed path normalization from TrieTree entirely. The trie now stores keys exactly as provided. Replaced `_normalizePath()` with `_isValidKey()` which only validates (rejects null bytes) but does not transform keys. Callers are responsible for any normalization.

#### 2. URI Resolution Concatenates Location Path with Decoded Path
**File:** `srcs/Http/HttpURI.cpp` (line ~219)

**Bug:** The path resolution concatenates root + location path + decoded path:
```cpp
std::string fullPath = *root + "/" + location->getLocationPath() + "/" + _decodedPath;
```

**Impact:** When requesting `/upload.php` and it incorrectly matches location `/upload/`:
- `root` = `/var/www/html`
- `location->getLocationPath()` = `/upload`
- `_decodedPath` = `/upload.php`
- Result: `/var/www/html/upload/upload.php` (incorrect - should be `/var/www/html/upload.php`)

**Status:** With bug #1 fixed, this issue no longer occurs for the trailing slash case. However, the concatenation logic may still have edge cases.

#### 3. ~~Prefix Match Finds Wrong Location for Similar Paths~~ (FIXED)
**File:** `srcs/Config/Server.cpp` (line ~265)

**Was:** The `findLongestPrefix()` in TrieTree returned any location whose normalized path was a prefix of the request path. Combined with bug #1 (trailing slash stripping), this caused `/upload.php` to match `/upload/` location instead of `/` location.

**Fix:** With trailing slashes now preserved in the trie, `/upload/` is stored as `/upload/` (not `/upload`), so it no longer matches `/upload.php` since `/upload/` is not a prefix of `/upload.php`.

## File Locations

- **Test configs:** `configs/*.conf`
- **Web root:** `www/` (static files, CGI scripts in `cgi-bin/`)
- **Logs:** `logs/` (session-based logging)
- **Architecture docs:** `Project_resources/webserv_architecture_flowchart.md`
