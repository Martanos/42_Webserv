# webserv

A http/1.1 compliant static file server

---- Key Design Considerations ----
	
	Single Event Loop Pattern
	
	-	One poll() call manages all I/O
		Non-blocking operations only
		State machines track progress
	
	Observer Pattern
	
	-	poll() "observes" file descriptor events
		Different handlers for different event types
	
	State Machine Pattern
	
	-	Each client maintains internal state
		State transitions drive the processing flow
	
	Factory Pattern
	
	-	Route matching creates appropriate handlers
		Different response generators for different content types

	Other considerations:
	
	 - Non-blocking: Never stall on I/O operations
	 - Scalable: Ability to handle concurrent connections
	 - Robustness: Proper error handling and resource cleanup
	 - Maintainability: Code design should show clear seperation of concerns

---- Pseudo classes ----

	class Server {
	    std::vector<int> listening_sockets;
	    std::map<int, Client*> clients;
	    Config config;
	
	    void run(); // Main event loop
	};
	
	class Client {
	    enum State { READING, PROCESSING, SENDING, DONE };
	
	    int socket_fd;
	    State current_state;
	    std::string request_buffer;
	    HttpRequest request;
	    HttpResponse response;
	
	    void handle_read();
	    void handle_write();
	};
	
	class HttpRequest {
	    std::string method;
	    std::string uri;
	    std::string version;
	    std::map<std::string, std::string> headers;
	    std::string body;
	};
	
	class HttpResponse {
	    int status_code;
	    std::map<std::string, std::string> headers;
	    std::string body;
	    size_t bytes_sent;
	};

---- Project flow ----

	1. Program intialisation
	
	main()
	├── Parse command line arguments [config_file]
	├── Load & Parse Configuration File
	│   ├── Parse server blocks (host:port combinations)
	│   ├── Parse routes and their rules
	│   ├── Parse error pages, body size limits
	│   └── Validate configuration
	├── Create Server instances for each host:port
	├── Initialize listening sockets (non-blocking)
	├── Bind sockets to addresses
	└── Start listening on all sockets
	
	2. Main event loop
	
	while (server_running) {
	    // SINGLE poll() call for ALL file descriptors
	    poll(all_file_descriptors, timeout);
	
	    for each fd that has activity:
	        if (fd is listening socket):
	            ├── Accept new connection
	            ├── Set client socket to non-blocking
	            ├── Create Client object
	            └── Add client fd to poll list
	
	        else if (fd is client socket):
	            if (POLLIN - readable):
	                ├── Read data from client
	                ├── Parse HTTP request incrementally
	                ├── Update client state machine
	                └── Process complete requests
	
	            if (POLLOUT - writable):
	                ├── Send response data
	                ├── Update client state machine
	                └── Close connection if response complete
	
	            if (POLLERR/POLLHUP):
	                ├── Handle client disconnection
	                └── Clean up client resources
	}
	
	3. Client states
	
	Each client connection goes through these states
	
	CLIENT_READING_REQUEST
	├── Read request line (GET /path HTTP/1.1)
	├── Read headers
	├── Read body (if POST/PUT)
	└── Transition to CLIENT_PROCESSING
	
	CLIENT_PROCESSING
	├── Route matching based on URI
	├── Method validation (GET/POST/DELETE)
	├── Generate appropriate response
	│   ├── Static file serving
	│   ├── CGI execution
	│   ├── Directory listing
	│   └── Error responses
	└── Transition to CLIENT_SENDING_RESPONSE
	
	CLIENT_SENDING_RESPONSE
	├── Send status line
	├── Send headers
	├── Send body (file/CGI output/error page)
	└── Transition to CLIENT_DONE or keep-alive
	
	CLIENT_DONE
	├── Clean up resources
	└── Remove from poll list
	
	4. Request Processing Pipeline
	
	HTTP Request Received
	├── Parse Request Line
	│   ├── Extract method (GET/POST/DELETE)
	│   ├── Extract URI path
	│   └── Extract HTTP version
	│
	├── Parse Headers
	│   ├── Content-Length
	│   ├── Content-Type
	│   ├── Host
	│   └── Other headers
	│
	├── Route Matching
	│   ├── Find matching server block
	│   ├── Find matching location/route
	│   └── Apply route configuration
	│
	├── Method & Permission Validation
	│   ├── Check if method allowed for route
	│   ├── Check file permissions
	│   └── Handle redirects
	│
	├── Content Generation
	│   ├── Static Files → read file, set MIME type
	│   ├── Directory Listing → generate HTML index
	│   ├── CGI Scripts → fork, exec, pipe I/O
	│   ├── File Upload → save to configured location
	│   └── Errors → generate error page
	│
	└── Response Generation
	    ├── Status code & reason phrase
	    ├── Response headers
	    └── Response body

---- Error handling strategy ----

	At Every Level:
	├── Network errors (EWOULDBLOCK, EPIPE, etc.)
	├── HTTP parsing errors (malformed requests)
	├── File system errors (404, 403, etc.)
	├── CGI errors (execution failures)
	├── Resource limits (body size, timeouts)
	└── Memory allocation failures
	
	Response:
	├── Log the error appropriately
	├── Generate proper HTTP error response
	├── Clean up resources
	└── Continue serving other 

---- Program termination strategy ----

	Signal Handler (SIGINT/SIGTERM):
	├── Set server_running = false
	├── Close all client connections gracefully
	├── Close listening sockets
	├── Clean up all resources
	└── Exit cleanly
