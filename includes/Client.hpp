// Client.hpp
#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include "Server.hpp"
#include "Logger.hpp"
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

// This class is used to handle the client connection and process the HTTP request
class Client
{
public:
	enum State
	{
		CLIENT_READING_REQUEST,
		CLIENT_PROCESSING_REQUEST,
		CLIENT_SENDING_RESPONSE,
		CLIENT_DONE,
		CLIENT_ERROR
	};

private:
	int _socketFd;
	struct sockaddr_in _clientAddr;
	State _currentState;

	HttpRequest _request;
	HttpResponse _response;
	const Server *_server;

	std::string _readBuffer;
	time_t _lastActivity;
	bool _keepAlive;

public:
	Client();
	Client(int socketFd, const struct sockaddr_in &clientAddr);
	Client(const Client &other);
	Client &operator=(const Client &other);
	~Client();

	// Core operations
	void handleRead();
	void handleWrite();
	void processRequest();

	// State management
	State getCurrentState() const { return _currentState; }
	void setState(State newState) { _currentState = newState; }
	bool isTimedOut() const;
	void updateActivity();

	// Getters
	int getSocketFd() const { return _socketFd; }
	const std::string &getClientIP() const;
	const Server *getServer() const { return _server; }
	void setServer(const Server *server) { _server = server; }

	// Connection management
	bool shouldClose() const;
	void closeConnection();

private:
	void _processHTTPRequest();
	void _generateErrorResponse(int statusCode, const std::string &message = "");
	void _generateFileResponse(const std::string &filePath);
	void _generateDirectoryListing(const std::string &dirPath);
	bool _isMethodAllowed(const std::string &method) const;
	std::string _resolveFilePath(const std::string &uri) const;
	void _handleCGIRequest();
	void _handleFileUpload();
};

#endif /* ********************************************************** CLIENT_H */
