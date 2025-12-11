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

CGI execution for dynamic content (Python scripts in `www/cgi-bin/`).

### CGI Flow
```
GetMethodHandler::_serveFile()
    └── CgiHandler::execute(scriptPath, request, response, &location, server)
            ├── CgiEnv::_transposeData() → Build CGI environment variables
            ├── CgiExecutor::execute()   → Fork/exec with pipes, timeout handling
            └── CgiResponse::parseOutput() → Parse script output into HttpResponse
```

### Key Classes
| Class | Responsibility |
|-------|----------------|
| `CgiHandler` | Orchestrator - coordinates env setup, execution, response parsing |
| `CgiEnv` | Builds CGI environment variables (REQUEST_METHOD, QUERY_STRING, etc.) |
| `CgiExecutor` | Fork/exec, pipe management (stdin/stdout/stderr), timeout (default 30s) |
| `CgiResponse` | Parses CGI output (headers + body), handles NPH mode, populates HttpResponse |

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
