# WebServ Program Flow

## Overview
This document describes the flow of the webserv program across its different folders/modules. The current main.cpp only parses and prints configurations (the actual server launch is commented as pseudo-code), but the flow below reflects the intended architecture based on the implemented components.

```text
WebServ Program Flow
====================

[Main Entry Point - srcs/main.cpp]
├── Logger initialization
├── Parse command line arguments
└── ConfigParser: parse configuration file
    ├── Creates ServerConfig objects (server blocks)
    ├── Creates LocationConfig objects (location blocks within servers)
    └── Print all parsed configurations (current output)

[Server Initialization Flow - Not yet implemented in main, but prepared]
ConfigParser (1.ConfigParser/ parsed configs ↓
├── ServerMap (2.ServerMap/): Convert ServerConfig to internal Server objects
│   ├── Server: Contains settings like root, ports, indexes, error pages
│   ├── Location: Contains URI-matching rules, methods, CGI settings, uploads
│   └── ServerMap: Maps ListeningSocket (host:port) → vector of Servers
└── ServerManager (3.ServerManager/): Manages the server lifecycle
    ├── Create EpollManager for event handling
    ├── Create ListeningSocket objects for each configured port
    ├── ServerManager.run(): Main event loop
    │   ├── Add all listening sockets to epoll
    │   └── Epoll.wait() for events [MAIN LOOP ENTERS HERE]
    │       ├── New connections on listening sockets:
    │       │   ├── ListeningSocket.accept() → client file descriptor
    │       │   ├── Create Client object (4.Client/)
    │       │   └── Add client FD to epoll (EPOLLIN for reading requests)
    │       └── Client events (4.Client/):
    │           ├── Client.handleEvent(): Process epoll event
    │           ├── Client.readRequest():
    │           │   ├── RingBuffer (Wrappers/): Buffer incoming data
    │           │   └── Parse HTTP request (5.HTTPmanagement/)
    │           │       ├── HttpURI: Parse method, URI, version
    │           │       ├── HttpHeaders: Parse headers, handle content-length/chunked
    │           │       ├── HttpBody: Parse body (chunked or content-length)
    │           │       └── HttpRequest: Aggregate parsed components
    │           ├── Client._identifyServer(): Match client to Server object
    │           ├── Client._processHTTPRequest():
    │           │   └── RequestRouter (MethodHandlers/): Route request
    │           │       ├── matchLocation(): Find best Location match for URI
    │           │       └── Check if method allowed in Location
    │           ├── Method handler execution (MethodHandlers/):
    │           │   ├── MethodHandlerFactory: Get appropriate handler (GET/POST/DELETE)
    │           │   ├── IMethodHandler: Base class with utilities
    │           │   ├── GetMethodHandler: Serve static files or directory listings
    │           │   ├── PostMethodHandler: Handle file uploads or CGI
    │           │   └── DeleteMethodHandler: Delete files (if allowed)
    │           └── Generate response (5.HTTPmanagement/):
    │               └── HttpResponse: Set status, headers, body
    └── Client.sendResponse():
        └── Write response to client socket

[Sub-module Utilities - Wrappers/srcs/Wrappers/]
├── FileDescriptor: RAII wrapper for socket/file descriptors
├── RingBuffer: Efficient data buffering for network I/O
├── Logger: Centralized logging system
├── AddrInfo: Socket address resolution
├── SocketAddress: sockaddr_in encapsulation
├── FileManager: File system operations
├── SafeBuffer: Thread-safe buffer (if needed)
└── Others as needed...

[CGI Execution - cgiexec/]
└── When POST/GET hits CGI-enabled location:
    ├── CgiHandler: Set up CGI environment and execute
    ├── CgiEnv: Build CGI environment variables
    ├── CgiExecutor: Fork process for CGI script execution
    └── CgiResponse: Parse CGI output back to HTTP response
```

## Key Interactions Across Folders
- **Config → ServerMap → ServerManager**: Configuration drives server map creation and initialization.
- **ServerManager → Client (via EpollManager)**: Central event loop delegates client-specific processing.
- **Client → HTTPmanagement**: HTTP parsing and response construction.
- **Client → MethodHandlers (via RequestRouter)**: Request routing and domain logic (file serving, uploads, CGI).
- **MethodHandlers → CGI (when applicable)**: Offload to CGI for dynamic content.
- **MethodHandlers → Wrappers**: Use utilities for file I/O, logging, buffers.
- **HTTPmanagement ←→ Wrappers**: RingBuffer enables streaming HTTP parsing.

## Architecture Notes
The architecture follows an event-driven, non-blocking model with epoll(7) multiplexing, supporting multiple servers/ports and efficient connection handling. Each folder encapsulates a clear responsibility, promoting modularity.
