#include "ConfigValidation.hpp"
#include <vector>
#include <sstream>
#include <cctype>
#include <cstdlib>
#include <algorithm>
#include <set>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigValidation::ConfigValidation(AST::Config &cfg) : _cfg(&cfg)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigValidation::~ConfigValidation()
{
}

/*
** --------------------------------- METHODS ----------------------------------
*/

void ConfigValidation::validate()
{
	// First pass: validate each server block
	for (std::vector<AST::Server>::iterator it = _cfg->servers.begin(); it != _cfg->servers.end();)
	{
		if (it->directives.empty() && it->locations.empty())
		{
			Logger::warning("Ignoring empty server block line: " + std::to_string(it->line) + " col: " + std::to_string(it->column));
			it = _cfg->servers.erase(it);
		}
		else
		{
			validateServerBlock(*it);
			++it;
		}
	}

	// Second pass: check for duplicate server configurations
	validateServerUniqueness();
}

void ConfigValidation::validateServerBlock(AST::Server &server)
{
	// Check for required listen directive
	bool hasListen = false;
	for (std::vector<AST::Directive>::const_iterator it = server.directives.begin(); it != server.directives.end(); ++it)
	{
		if (it->name == "listen")
		{
			hasListen = true;
			break;
		}
	}

	if (!hasListen)
	{
		Logger::warning("Server block missing required listen directive line: " + std::to_string(server.line) + " col: " + std::to_string(server.column));
	}

	// Validate server-level directives
	for (std::vector<AST::Directive>::iterator it = server.directives.begin(); it != server.directives.end();)
	{
		if (validateDirective(*it))
		{
			++it;
		}
		else
		{
			it = server.directives.erase(it);
		}
	}

	// Validate locations
	for (std::vector<AST::Location>::iterator it = server.locations.begin(); it != server.locations.end();)
	{
		if (validateLocation(*it))
		{
			++it;
		}
		else
		{
			it = server.locations.erase(it);
		}
	}

	// Check for duplicate locations within this server
	validateLocationUniqueness(server);
}

bool ConfigValidation::validateDirective(AST::Directive &directive)
{
	// Basic validation - check if directive has required arguments
	if (directive.name.empty())
	{
		Logger::warning("Ignoring empty directive name line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// Validate based on directive type
	if (directive.name == "listen")
		return validateListenDirective(directive);
	else if (directive.name == "server_name")
		return validateServerNameDirective(directive);
	else if (directive.name == "root")
		return validateRootDirective(directive);
	else if (directive.name == "index")
		return validateIndexDirective(directive);
	else if (directive.name == "autoindex")
		return validateAutoindexDirective(directive);
	else if (directive.name == "client_max_uri_size")
		return validateClientMaxUriSizeDirective(directive);
	else if (directive.name == "client_max_headers_size")
		return validateClientMaxHeadersSizeDirective(directive);
	else if (directive.name == "client_max_body_size")
		return validateClientMaxBodySizeDirective(directive);
	else if (directive.name == "error_page")
		return validateErrorPageDirective(directive);
	else if (directive.name == "keep_alive")
		return validateKeepAliveDirective(directive);
	else if (directive.name == "allowed_methods")
		return validateAllowedMethodsDirective(directive);
	else if (directive.name == "return")
		return validateReturnDirective(directive);
	else if (directive.name == "cgi_path")
		return validateCgiPathDirective(directive);
	else if (directive.name == "cgi_param")
		return validateCgiParamDirective(directive);
	else
	{
		Logger::warning("Ignoring unknown directive '" + directive.name + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}
}

bool ConfigValidation::validateLocation(AST::Location &location)
{
	// Basic validation - check if location has path and directives
	if (location.path.empty())
	{
		Logger::warning("Ignoring location block - empty path line: " + std::to_string(location.line) + " col: " + std::to_string(location.column));
		return false;
	}

	// Check for required allowed_methods directive
	bool hasAllowedMethods = false;
	for (std::vector<AST::Directive>::const_iterator it = location.directives.begin(); it != location.directives.end(); ++it)
	{
		if (it->name == "allowed_methods")
		{
			hasAllowedMethods = true;
			break;
		}
	}

	if (!hasAllowedMethods)
	{
		Logger::warning("Location block missing required allowed_methods directive line: " + std::to_string(location.line) + " col: " + std::to_string(location.column));
	}

	// Validate location directives
	for (std::vector<AST::Directive>::iterator it = location.directives.begin(); it != location.directives.end();)
	{
		if (validateDirective(*it))
		{
			++it;
		}
		else
		{
			it = location.directives.erase(it);
		}
	}

	// Check for conflicting directives (return vs cgi_path)
	validateLocationConflicts(location);

	return true;
}

/*
** --------------------------------- SPECIFIC DIRECTIVE VALIDATORS ----------------------------------
*/

bool ConfigValidation::validateListenDirective(AST::Directive &directive)
{
	// listen: port | host:port | port1 port2 port3 | host1:port1 host2:port2 host3:port3
	if (directive.args.empty())
	{
		Logger::warning("Ignoring listen directive - requires at least one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	for (size_t i = 0; i < directive.args.size(); ++i)
	{
		const std::string &arg = directive.args[i];
		if (arg.find(':') != std::string::npos)
		{
			// host:port format
			size_t colonPos = arg.find(':');
			std::string host = arg.substr(0, colonPos);
			std::string port = arg.substr(colonPos + 1);

			if (host.empty() || port.empty())
			{
				Logger::warning("Ignoring listen directive - invalid host:port format '" + arg + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
				return false;
			}

			if (!isValidPort(port))
			{
				Logger::warning("Ignoring listen directive - invalid port '" + port + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
				return false;
			}
		}
		else
		{
			// port only
			if (!isValidPort(arg))
			{
				Logger::warning("Ignoring listen directive - invalid port '" + arg + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
				return false;
			}
		}
	}
	return true;
}

bool ConfigValidation::validateServerNameDirective(AST::Directive &directive)
{
	// server_name: name1 name2 name3 (space-separated)
	// Optional directive, no validation needed for empty args
	return true;
}

bool ConfigValidation::validateRootDirective(AST::Directive &directive)
{
	// root: /absolute/path/to/directory
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring root directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &path = directive.args[0];
	if (path.empty())
	{
		Logger::warning("Ignoring root directive - path cannot be empty line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// Note: In a real implementation, you might want to check if path exists
	return true;
}

bool ConfigValidation::validateIndexDirective(AST::Directive &directive)
{
	// index: file1 file2 file3 (space-separated)
	// Optional directive, no validation needed for empty args
	return true;
}

bool ConfigValidation::validateAutoindexDirective(AST::Directive &directive)
{
	// autoindex: on | off
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring autoindex directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &value = directive.args[0];
	if (value != "on" && value != "off")
	{
		Logger::warning("Ignoring autoindex directive - must be 'on' or 'off', got '" + value + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}
	return true;
}

bool ConfigValidation::validateClientMaxUriSizeDirective(AST::Directive &directive)
{
	// client_max_uri_size: number[K|M|G]
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring client_max_uri_size directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return validateSizeArgument(directive.args[0], "client_max_uri_size", directive);
}

bool ConfigValidation::validateClientMaxHeadersSizeDirective(AST::Directive &directive)
{
	// client_max_headers_size: number[K|M|G]
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring client_max_headers_size directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return validateSizeArgument(directive.args[0], "client_max_headers_size", directive);
}

bool ConfigValidation::validateClientMaxBodySizeDirective(AST::Directive &directive)
{
	// client_max_body_size: number[K|M|G]
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring client_max_body_size directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return validateSizeArgument(directive.args[0], "client_max_body_size", directive);
}

bool ConfigValidation::validateErrorPageDirective(AST::Directive &directive)
{
	// error_page: code1 code2 /path/to/file
	if (directive.args.size() < 2)
	{
		Logger::warning("Ignoring error_page directive - requires at least 2 arguments line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// Validate status codes (all but last argument)
	for (size_t i = 0; i < directive.args.size() - 1; ++i)
	{
		if (!isValidStatusCode(directive.args[i]))
		{
			Logger::warning("Ignoring error_page directive - invalid status code '" + directive.args[i] + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
			return false;
		}
	}

	// Last argument should be a path
	const std::string &path = directive.args.back();
	if (path.empty())
	{
		Logger::warning("Ignoring error_page directive - path cannot be empty line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return true;
}

bool ConfigValidation::validateKeepAliveDirective(AST::Directive &directive)
{
	// keep_alive: keepalive | close
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring keep_alive directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &value = directive.args[0];
	if (value != "keepalive" && value != "close")
	{
		Logger::warning("Ignoring keep_alive directive - must be 'keepalive' or 'close', got '" + value + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}
	return true;
}

bool ConfigValidation::validateAllowedMethodsDirective(AST::Directive &directive)
{
	// allowed_methods: GET POST DELETE (space-separated)
	if (directive.args.empty())
	{
		Logger::warning("Ignoring allowed_methods directive - requires at least one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	for (size_t i = 0; i < directive.args.size(); ++i)
	{
		const std::string &method = directive.args[i];
		if (method != "GET" && method != "POST" && method != "DELETE")
		{
			Logger::warning("Ignoring allowed_methods directive - invalid method '" + method + "'. Only GET, POST, DELETE allowed line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
			return false;
		}
	}
	return true;
}

bool ConfigValidation::validateReturnDirective(AST::Directive &directive)
{
	// return: 301|302 URL
	if (directive.args.size() != 2)
	{
		Logger::warning("Ignoring return directive - requires exactly 2 arguments line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &code = directive.args[0];
	if (code != "301" && code != "302")
	{
		Logger::warning("Ignoring return directive - status code must be 301 or 302, got '" + code + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// URL validation could be added here
	return true;
}

bool ConfigValidation::validateCgiPathDirective(AST::Directive &directive)
{
	// cgi_path: /absolute/path/to/executable
	if (directive.args.size() != 1)
	{
		Logger::warning("Ignoring cgi_path directive - requires exactly one argument line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &path = directive.args[0];
	if (path.empty())
	{
		Logger::warning("Ignoring cgi_path directive - path cannot be empty line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return true;
}

bool ConfigValidation::validateCgiParamDirective(AST::Directive &directive)
{
	// cgi_param: PARAM_NAME value
	if (directive.args.size() != 2)
	{
		Logger::warning("Ignoring cgi_param directive - requires exactly 2 arguments line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	const std::string &paramName = directive.args[0];
	if (paramName.empty())
	{
		Logger::warning("Ignoring cgi_param directive - parameter name cannot be empty line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	return true;
}

/*
** --------------------------------- HELPER METHODS ----------------------------------
*/

bool ConfigValidation::isValidPort(const std::string &port)
{
	if (port.empty())
		return false;

	for (size_t i = 0; i < port.size(); ++i)
	{
		if (!std::isdigit(port[i]))
			return false;
	}

	int portNum = std::atoi(port.c_str());
	return portNum >= 1 && portNum <= 65535;
}

bool ConfigValidation::isValidStatusCode(const std::string &code)
{
	if (code.empty())
		return false;

	for (size_t i = 0; i < code.size(); ++i)
	{
		if (!std::isdigit(code[i]))
			return false;
	}

	int statusCode = std::atoi(code.c_str());
	return statusCode >= 400 && statusCode <= 599;
}

bool ConfigValidation::validateSizeArgument(const std::string &arg, const std::string &directiveName, const AST::Directive &directive)
{
	if (arg.empty())
	{
		Logger::warning("Ignoring " + directiveName + " directive - argument cannot be empty line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// Parse number and optional suffix
	size_t i = 0;
	while (i < arg.size() && std::isdigit(arg[i]))
		++i;

	if (i == 0)
	{
		Logger::warning("Ignoring " + directiveName + " directive - requires a positive number, got '" + arg + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
		return false;
	}

	// Check for valid suffix
	if (i < arg.size())
	{
		std::string suffix = arg.substr(i);
		if (suffix != "K" && suffix != "M" && suffix != "G")
		{
			Logger::warning("Ignoring " + directiveName + " directive - suffix must be K, M, or G, got '" + suffix + "' line: " + std::to_string(directive.line) + " col: " + std::to_string(directive.column));
			return false;
		}
	}

	return true;
}

/*
** --------------------------------- ENHANCED VALIDATION METHODS ----------------------------------
*/

void ConfigValidation::validateServerUniqueness()
{
	std::set<std::string> seenListenPairs;

	for (std::vector<AST::Server>::iterator serverIt = _cfg->servers.begin(); serverIt != _cfg->servers.end(); ++serverIt)
	{
		for (std::vector<AST::Directive>::const_iterator dirIt = serverIt->directives.begin(); dirIt != serverIt->directives.end(); ++dirIt)
		{
			if (dirIt->name == "listen")
			{
				for (size_t i = 0; i < dirIt->args.size(); ++i)
				{
					std::string listenPair = dirIt->args[i];
					if (seenListenPairs.find(listenPair) != seenListenPairs.end())
					{
						Logger::warning("Ignoring duplicate listen directive '" + listenPair + "' line: " + std::to_string(dirIt->line) + " col: " + std::to_string(dirIt->column));
					}
					else
					{
						seenListenPairs.insert(listenPair);
					}
				}
			}
		}
	}
}

void ConfigValidation::validateLocationUniqueness(AST::Server &server)
{
	std::set<std::string> seenPaths;

	for (std::vector<AST::Location>::iterator locIt = server.locations.begin(); locIt != server.locations.end(); ++locIt)
	{
		if (seenPaths.find(locIt->path) != seenPaths.end())
		{
			Logger::warning("Ignoring duplicate location path '" + locIt->path + "' line: " + std::to_string(locIt->line) + " col: " + std::to_string(locIt->column));
		}
		else
		{
			seenPaths.insert(locIt->path);
		}
	}
}

void ConfigValidation::validateLocationConflicts(AST::Location &location)
{
	bool hasReturn = false;
	bool hasCgiPath = false;
	bool hasUploadPath = false;

	// Check for conflicting directives
	for (std::vector<AST::Directive>::const_iterator it = location.directives.begin(); it != location.directives.end(); ++it)
	{
		if (it->name == "return")
			hasReturn = true;
		else if (it->name == "cgi_path")
			hasCgiPath = true;
		else if (it->name == "upload_path")
			hasUploadPath = true;
	}

	// Check for conflicts
	if (hasReturn && hasCgiPath)
	{
		Logger::warning("Location has conflicting directives: return and cgi_path cannot coexist line: " + std::to_string(location.line) + " col: " + std::to_string(location.column));
	}

	if (hasReturn && hasUploadPath)
	{
		Logger::warning("Location has conflicting directives: return and upload_path cannot coexist line: " + std::to_string(location.line) + " col: " + std::to_string(location.column));
	}

	// Check for cgi_param without cgi_path
	bool hasCgiParam = false;
	for (std::vector<AST::Directive>::const_iterator it = location.directives.begin(); it != location.directives.end(); ++it)
	{
		if (it->name == "cgi_param")
		{
			hasCgiParam = true;
			break;
		}
	}

	if (hasCgiParam && !hasCgiPath)
	{
		Logger::warning("Location has cgi_param directive but no cgi_path directive line: " + std::to_string(location.line) + " col: " + std::to_string(location.column));
	}
}