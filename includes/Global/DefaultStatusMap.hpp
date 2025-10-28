#ifndef STATUSMAP_HPP
#define STATUSMAP_HPP

#include "Logger.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

// Utility class that generates a static status map for the whole server
// It returns a map key:value pair with key being response number and value
// being a static response page If it contains a vector of size 2, the first
// element is the message and the second element is the body If it contains a
// vector of size 1, the first element is the message and it must never contain
// a body This map is to be used as a fallback if the server does not define its
// own body
// TODO: Unknown status codes should return a body that explains the status code
class DefaultStatusMap
{
private:
	DefaultStatusMap();
	DefaultStatusMap(DefaultStatusMap const &src);

	DefaultStatusMap &operator=(DefaultStatusMap const &rhs);

	static std::map<int, std::string> &getDefaultStatusMap()
	{
		static std::map<int, std::string> instance;
		return (instance);
	}

	static bool &getInitialized()
	{
		static bool instance = false;
		return (instance);
	}

	// Informational responses - MUST NOT have response body (RFC 9110)
	static void init100Statuses()
	{
		getDefaultStatusMap()[100] = generateStatusInfo(100, "Continue", false,
														false); // Server received request headers and is
																// willing to proceed with request body
		getDefaultStatusMap()[101] = generateStatusInfo(101, "Switching Protocols", false,
														false); // Server is switching protocols as requested by client
																// (e.g., HTTP to WebSocket)
		getDefaultStatusMap()[102] = generateStatusInfo(102, "Processing", false,
														false); // Server is processing request but no
																// response available yet (WebDAV)
		getDefaultStatusMap()[103] = generateStatusInfo(103, "Early Hints", false,
														false); // Server sending preliminary response
																// headers before final response
	}

	// Success responses - Bodies depend on specific use case
	static void init200Statuses()
	{
		getDefaultStatusMap()[200] = generateStatusInfo(200, "OK", true, false); // Standard successful response -
																				 // usually contains requested resource
		getDefaultStatusMap()[201] = generateStatusInfo(201, "Created", true,
														false); // Resource successfully created - often
																// contains created resource details
		getDefaultStatusMap()[202] = generateStatusInfo(202, "Accepted", true,
														false); // Request accepted for async processing
																// - contains processing status
		getDefaultStatusMap()[203] = generateStatusInfo(203, "Non-Authoritative Information", true,
														false); // Success but info from cache/proxy, not origin server
		getDefaultStatusMap()[204] = generateStatusInfo(204, "No Content", false,
														false); // Success but no content to return -
																// MUST NOT have response body
		getDefaultStatusMap()[205] = generateStatusInfo(205, "Reset Content", true,
														false); // Success, client should reset document
																// view - usually minimal body
		getDefaultStatusMap()[206] = generateStatusInfo(206, "Partial Content", true,
														false); // Partial content delivered for range
																// requests - contains requested ranges
	}

	// Redirection responses - Should contain short HTML with link to new
	// location
	static void init300Statuses()
	{
		getDefaultStatusMap()[300] =
			generateStatusInfo(300, "Multiple Choices", true,
							   false); // Multiple representations available - body lists options
		getDefaultStatusMap()[301] = generateStatusInfo(301, "Moved Permanently", true,
														false); // Resource permanently moved - body
																// contains link for old browsers
		getDefaultStatusMap()[302] =
			generateStatusInfo(302, "Found", true, false); // Resource temporarily at different
														   // location - body contains temp link
		getDefaultStatusMap()[303] = generateStatusInfo(303, "See Other", true,
														false); // Response found elsewhere, use GET to
																// retrieve - body explains redirect
		getDefaultStatusMap()[304] = generateStatusInfo(304, "Not Modified", false,
														false); // Resource not modified since last
																// request - MUST NOT have response body
		getDefaultStatusMap()[305] = generateStatusInfo(305, "Use Proxy", true,
														false); // Must access resource through specified
																// proxy - body contains proxy info
		getDefaultStatusMap()[307] = generateStatusInfo(307, "Temporary Redirect", true,
														false); // Temporary redirect preserving request
																// method - body contains temp link
		getDefaultStatusMap()[308] = generateStatusInfo(308, "Permanent Redirect", true,
														false); // Permanent redirect preserving request
																// method - body contains new link
	}

	// Client error responses - Should contain explanation of the error
	static void init400Statuses()
	{
		getDefaultStatusMap()[400] =
			generateStatusInfo(400, "Bad Request", true,
							   true); // Malformed request syntax - body explains what was wrong
		getDefaultStatusMap()[401] =
			generateStatusInfo(401, "Unauthorized", true,
							   true); // Authentication required - body contains login instructions
		getDefaultStatusMap()[402] = generateStatusInfo(402, "Payment Required", true,
														true); // Payment required (rarely used) - body
															   // explains payment needed
		getDefaultStatusMap()[403] = generateStatusInfo(403, "Forbidden", true, true); // Server understood but refuses
																					   // access - body explains denial
		getDefaultStatusMap()[404] =
			generateStatusInfo(404, "Not Found", true, true); // Resource not found - body contains
															  // user-friendly not found page
		getDefaultStatusMap()[405] = generateStatusInfo(405, "Method Not Allowed", true,
														true); // HTTP method not allowed for this
															   // resource - body lists allowed methods
		getDefaultStatusMap()[406] = generateStatusInfo(406, "Not Acceptable", true,
														true); // No content matching Accept headers -
															   // body lists available types
		getDefaultStatusMap()[407] = generateStatusInfo(407, "Proxy Authentication Required", true,
														true); // Proxy authentication required - body
															   // contains proxy auth instructions
		getDefaultStatusMap()[408] =
			generateStatusInfo(408, "Request Timeout", true,
							   true); // Server timeout waiting for request - body explains timeout
		getDefaultStatusMap()[409] = generateStatusInfo(409, "Conflict", true,
														true); // Request conflicts with current resource
															   // state - body explains conflict
		getDefaultStatusMap()[410] = generateStatusInfo(410, "Gone", true, true); // Resource permanently deleted - body
																				  // explains resource is gone
		getDefaultStatusMap()[411] =
			generateStatusInfo(411, "Length Required", true,
							   true); // Content-Length header required - body explains requirement
		getDefaultStatusMap()[412] = generateStatusInfo(412, "Precondition Failed", true,
														true); // Precondition headers failed - body
															   // explains which preconditions failed
		getDefaultStatusMap()[413] = generateStatusInfo(413, "Payload Too Large", true,
														true); // Request body too large - body explains size limits
		getDefaultStatusMap()[414] = generateStatusInfo(414, "URI Too Long", true,
														true); // Request URI too long - body explains URI length limits
		getDefaultStatusMap()[415] = generateStatusInfo(415, "Unsupported Media Type", true,
														true); // Media type not supported - body lists supported types
		getDefaultStatusMap()[416] = generateStatusInfo(416, "Range Not Satisfiable", true,
														true); // Range header cannot be satisfied - body
															   // contains valid range info
		getDefaultStatusMap()[417] = generateStatusInfo(417, "Expectation Failed", true,
														true); // Expect header cannot be met - body
															   // explains expectation failure
		getDefaultStatusMap()[418] = generateStatusInfo(418, "I'm a teapot", true,
														true); // April Fool's RFC joke - teapot cannot brew coffee
		getDefaultStatusMap()[421] =
			generateStatusInfo(421, "Misdirected Request", true,
							   true); // Request directed at server that cannot produce response
	}

	// Server error responses - Should contain explanation of server-side error
	static void init500Statuses()
	{
		getDefaultStatusMap()[500] =
			generateStatusInfo(500, "Internal Server Error", true,
							   true); // Generic server error - body contains generic error message
		getDefaultStatusMap()[501] = generateStatusInfo(501, "Not Implemented", true,
														true); // Server doesn't support functionality -
															   // body explains unsupported feature
		getDefaultStatusMap()[502] = generateStatusInfo(502, "Bad Gateway", true,
														true); // Invalid response from upstream server -
															   // body explains gateway error
		getDefaultStatusMap()[503] = generateStatusInfo(503, "Service Unavailable", true,
														true); // Server temporarily unavailable - body
															   // contains maintenance message
		getDefaultStatusMap()[504] =
			generateStatusInfo(504, "Gateway Timeout", true,
							   true); // Upstream server timeout - body explains gateway timeout
		getDefaultStatusMap()[505] =
			generateStatusInfo(505, "HTTP Version Not Supported", true,
							   true); // HTTP version not supported - body lists supported versions
		getDefaultStatusMap()[506] = generateStatusInfo(506, "Variant Also Negotiates", true,
														true); // Transparent content negotiation results
															   // in circular reference
	}

	// Init function
	static void initDefaultStatusMap()
	{
		if (getInitialized())
			return;
		getInitialized() = true;
		init100Statuses();
		init200Statuses();
		init300Statuses();
		init400Statuses();
		init500Statuses();
		Logger::log(Logger::DEBUG, "StatusMap: Initialized default status map");
	}

	// HTML Body generation - Nginx-style default error pages
	static std::string generateHTMLBody(const int &status, const std::string &message)
	{
		std::stringstream ss;

		// Generate error-specific content
		std::string errorContent = getErrorContent(status);

		ss << "<!DOCTYPE html>\n"
		   << "<html>\n"
		   << "<head>\n"
		   << "<title>" << status << " " << message << "</title>\n"
		   << "<style>\n"
		   << "    body {\n"
		   << "        width: 35em;\n"
		   << "        margin: 0 auto;\n"
		   << "        font-family: Tahoma, Verdana, Arial, sans-serif;\n"
		   << "    }\n"
		   << "</style>\n"
		   << "</head>\n"
		   << "<body>\n"
		   << "<h1>" << message << "</h1>\n"
		   << errorContent << "<hr>\n"
		   << "<center>42_Webserv/1.0</center>\n"
		   << "</body>\n"
		   << "</html>\n";

		return ss.str();
	}

	// Generate error-specific content based on status code
	static std::string getErrorContent(const int &status)
	{
		switch (status)
		{
		case 400:
			return "<p>The request could not be understood by the server due "
				   "to malformed syntax.</p>";
		case 401:
			return "<p>This server could not verify that you are authorized to "
				   "access the document requested.</p>";
		case 403:
			return "<p>You don't have permission to access this resource.</p>";
		case 404:
			return "<p>The requested URL was not found on this server.</p>";
		case 405:
			return "<p>The request method is not allowed for the requested "
				   "URL.</p>";
		case 413:
			return "<p>The request entity is too large.</p>";
		case 500:
			return "<p>The server encountered an internal error and was unable "
				   "to complete your request.</p>";
		case 501:
			return "<p>The server does not support the action requested.</p>";
		case 502:
			return "<p>The server received an invalid response from an "
				   "upstream server.</p>";
		case 503:
			return "<p>The server is temporarily unable to service your "
				   "request due to maintenance downtime or capacity "
				   "problems.</p>";
		case 504:
			return "<p>The server did not receive a timely response from an "
				   "upstream server.</p>";
		default:
			return "<p>The server encountered an internal error and was unable "
				   "to complete your request.</p>";
		}
	}

	// Status info generation
	static std::string generateStatusInfo(const int &status, const std::string &message, bool can_have_body,
										  bool generate_body)
	{
		std::string message_body = message;
		if (can_have_body && generate_body)
			message_body += generateHTMLBody(status, message);
		else if (can_have_body && !generate_body)
			message_body += "";
		{
			std::stringstream ss;
			ss << "Generated status info for " << status << " with message " << message << " and body "
			   << (can_have_body && generate_body ? "true" : "false");
			Logger::log(Logger::DEBUG, "StatusMap: " + ss.str());
		}
		return (message_body);
	}

public:
	~DefaultStatusMap()
	{
		getDefaultStatusMap().clear();
		getInitialized() = false;
	}

	static std::string getStatusInfo(const int &status)
	{
		initDefaultStatusMap();
		if (getDefaultStatusMap().find(status) == getDefaultStatusMap().end())
			return (getDefaultStatusMap()[501]);
		return (getDefaultStatusMap()[status]);
	}

	static std::string getStatusMessage(const int &status)
	{
		initDefaultStatusMap();
		if (getDefaultStatusMap().find(status) == getDefaultStatusMap().end())
			return ("Internal Server Error");
		return (getDefaultStatusMap()[status]);
	}

	static std::string getStatusBody(const int &status)
	{
		initDefaultStatusMap();
		if (getDefaultStatusMap().find(status) == getDefaultStatusMap().end())
			return (getDefaultStatusMap()[501]);
		else if (getDefaultStatusMap()[status].size() < 2)
			return ("");
		return (getDefaultStatusMap()[status]);
	}

	static bool hasStatusBody(const int &status)
	{
		initDefaultStatusMap();
		if (getDefaultStatusMap().find(status) == getDefaultStatusMap().end())
			return (false);
		return (getDefaultStatusMap()[status].size() > 1);
	}
};

#endif /* ***************************************************** STATUSMAP_H */
