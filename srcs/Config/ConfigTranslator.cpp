#include "../../includes/Config/ConfigTranslator.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <cctype>
#include <cstdlib>
#include <vector>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigTranslator::ConfigTranslator(const AST::ASTNode &ast)
{
	_translate(ast);
}

/*
** --------------------------------- DESTRUCTOR ---------------------------------
*/

ConfigTranslator::~ConfigTranslator()
{
}

/*
** --------------------------------- ACCESSORS ---------------------------------
*/

const std::vector<Server> &ConfigTranslator::getServers() const
{
	return _servers;
}

/*
** --------------------------------- TRANSLATION ---------------------------------
*/

// Main translation function
// Iterate through the AST and translate the server blocks
void ConfigTranslator::_translate(const AST::ASTNode &ast)
{
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::SERVER)
		{
			Server server = _translateServer(**it);
			if (!server.wasModified())
				Logger::warning("No valid members in server block" + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else if (server.getServerNames().isEmpty())
				Logger::warning("No server names in server block" + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else if (server.getSocketAddresses().empty())
				Logger::warning("No socket addresses in server block" + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else
				_servers.push_back(server);
		}
	}
}

/*
** --------------------------------- SERVER SPECIFIC HELPERS ---------------------------------
*/

Server ConfigTranslator::_translateServer(const AST::ASTNode &ast)
{
	Server server;
	// Traverse the server block and translate recognizable members
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::DIRECTIVE)
		{
			if ((*it)->value == "server_name")
				_translateServerName(**it, server);
			else if ((*it)->value == "listen")
				_translateListen(**it, server);
			else
				_translateDirective(it, server, "server");
		}
		else if ((*it)->type == AST::LOCATION)
		{
			Location location((*it)->value);
			_translateLocation(**it, location);
			if (location.wasModified())
				server.insertLocation(location);
			else
				Logger::warning("No valid members in location block" + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else
			Logger::warning("Unknown token in server block: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	// Inheritence step: propagate server-level members to locations that lack them
	for (TrieTree<Location>::iterator locIt = server.getLocations().begin(); locIt != server.getLocations().end();
		 ++locIt)
	{
		Location &location = *locIt;
		if (server.hasRootPathDirective() && !location.hasRootPathDirective())
			location.setRootPath(server.getRootPath());
		if (server.hasAutoIndexDirective() && !location.hasAutoIndexDirective())
			location.setAutoIndex(server.getAutoIndexValue());
		if (server.hasCgiPathDirective() && !location.hasCgiPathDirective())
			location.setCgiPath(server.getCgiPath());
		if (server.hasClientMaxBodySizeDirective() && !location.hasClientMaxBodySizeDirective())
			location.setClientMaxBodySize(server.getClientMaxBodySize());
		if (server.hasKeepAliveDirective() && !location.hasKeepAliveDirective())
			location.setKeepAlive(server.getKeepAliveValue());
		if (server.hasRedirectDirective() && !location.hasRedirectDirective())
			location.setRedirect(server.getRedirect());
		if (server.hasIndexDirective() && !location.hasIndexDirective())
			location.setIndexes(server.getIndexes());
		if (server.hasStatusPathDirective() && !location.hasStatusPathDirective())
			location.setStatusPaths(server.getStatusPaths());
		if (server.hasAllowedMethodsDirective() && !location.hasAllowedMethodsDirective())
			location.setAllowedMethods(server.getAllowedMethods());
	}
	// Default location block, only created if root path was set at server level
	if (!server.getLocation("/") && server.wasModified() && server.hasRootPathDirective())
	{
		Location rootLocation("/");
		if (server.hasRootPathDirective())
			rootLocation.setRootPath(server.getRootPath());
		if (server.hasAutoIndexDirective())
			rootLocation.setAutoIndex(server.getAutoIndexValue());
		if (server.hasCgiPathDirective())
			rootLocation.setCgiPath(server.getCgiPath());
		if (server.hasClientMaxBodySizeDirective())
			rootLocation.setClientMaxBodySize(server.getClientMaxBodySize());
		if (server.hasKeepAliveDirective())
			rootLocation.setKeepAlive(server.getKeepAliveValue());
		if (server.hasRedirectDirective())
			rootLocation.setRedirect(server.getRedirect());
		if (server.hasIndexDirective())
			rootLocation.setIndexes(server.getIndexes());
		if (server.hasStatusPathDirective())
			rootLocation.setStatusPaths(server.getStatusPaths());
		if (server.hasAllowedMethodsDirective())
			rootLocation.setAllowedMethods(server.getAllowedMethods());
		if (rootLocation.wasModified())
			server.insertLocation(rootLocation);
	}
	return server;
}

// Server name verification
void ConfigTranslator::_translateServerName(const AST::ASTNode &directive, Server &server)
{
	for (std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin(); it != directive.children.end();
		 ++it)
	{
		try
		{
			if ((*it)->type == AST::ARG)
			{
				if ((*it)->value.empty())
					Logger::warning("Empty server name" + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
				else if (!server.hasServerName((*it)->value))
				{
					if (StrUtils::hasConsecutiveDots((*it)->value))
						Logger::warning("Consecutive dots in server name: " + (*it)->value +
											" line: " + StrUtils::toString<int>((*it)->line) +
											" column: " + StrUtils::toString<int>((*it)->column),
										__FILE__, __LINE__, __PRETTY_FUNCTION__);
					else if (StrUtils::hasSpaces((*it)->value))
						Logger::warning("Spaces in server name: " + (*it)->value +
											" line: " + StrUtils::toString<int>((*it)->line) +
											" column: " + StrUtils::toString<int>((*it)->column),
										__FILE__, __LINE__, __PRETTY_FUNCTION__);
					else if (StrUtils::hasControlCharacters((*it)->value))
						Logger::warning("Control characters in server name: " + (*it)->value +
											" line: " + StrUtils::toString<int>((*it)->line) +
											" column: " + StrUtils::toString<int>((*it)->column),
										__FILE__, __LINE__, __PRETTY_FUNCTION__);
					server.insertServerName((*it)->value);
				}
				else
					Logger::warning("Unknown token in server name block: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		catch (const std::exception &e)
		{
			Logger::error("Error translating server name: " + std::string(e.what()) +
							  " line: " + StrUtils::toString<int>(directive.line) +
							  " column: " + StrUtils::toString<int>((*it)->column),
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
}

// Translate listen directives into server members
void ConfigTranslator::_translateListen(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return Logger::warning("No arguments in listen directive" + directive.value +
									   " line: " + StrUtils::toString<int>(directive.line) +
									   " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
								   __FILE__, __LINE__, __PRETTY_FUNCTION__);
		else if ((*it)->type == AST::ARG)
		{
			SocketAddress socket((*it)->value);
			server.insertSocketAddress(socket);
		}
		else
			Logger::warning("Unknown token in listen directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
			Logger::warning("Extra argument in listen directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating listen directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

/*
** --------------------------------- LOCATION SPECIFIC HELPERS ---------------------------------
*/

// Location verification
void ConfigTranslator::_translateLocation(const AST::ASTNode &location_node, Location &location)
{
	try
	{
		for (std::vector<AST::ASTNode *>::const_iterator it = location_node.children.begin();
			 it != location_node.children.end(); ++it)
		{
			Logger::debug("Processing child: type=" + StrUtils::toString<int>((*it)->type) + ", value=" + (*it)->value,
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			if ((*it)->type == AST::DIRECTIVE)
			{
				_translateDirective(it, location, "location");
			}
			else
				Logger::warning("Unknown token in location block: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location block: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(location_node.line) +
						  " column: " + StrUtils::toString<int>(location_node.column),
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

/*
** --------------------------------- DIRECTIVE SPECIFIC TRANSLATION ---------------------------------
*/

// Routes a directive to the appropriate translation helper
void ConfigTranslator::_translateDirective(std::vector<AST::ASTNode *>::const_iterator &directive,
										   Directives &directives, std::string context)
{
	if ((*directive)->value == "root")
		_translateRootPathDirective(**directive, directives, context);
	else if ((*directive)->value == "autoindex")
		_translateAutoindexDirective(**directive, directives, context);
	else if ((*directive)->value == "cgi_path")
		_translateCgiPathDirective(**directive, directives, context);
	else if ((*directive)->value == "client_max_body_size")
		_translateClientMaxBodySizeDirective(**directive, directives, context);
	else if ((*directive)->value == "keep_alive")
		_translateKeepAliveDirective(**directive, directives, context);
	else if ((*directive)->value == "redirect")
		_translateRedirectDirective(**directive, directives, context);
	else if ((*directive)->value == "index")
		_translateIndexDirective(**directive, directives, context);
	else if ((*directive)->value == "status_page")
		_translateStatusPathDirective(**directive, directives, context);
	else if ((*directive)->value == "allowed_methods")
		_translateAllowedMethodsDirective(**directive, directives, context);
	else
		Logger::warning("Unknown directive in " + context + " block: " + (*directive)->value +
							" line: " + StrUtils::toString<int>((*directive)->line) +
							" column: " + StrUtils::toString<int>((*directive)->column) + " skipping...",
						__FILE__, __LINE__, __PRETTY_FUNCTION__);
}

void ConfigTranslator::_translateRootPathDirective(const AST::ASTNode &directive, Directives &directives,
												   std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " root directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			std::string error = StrUtils::validateDirectoryPath((*it)->value, context + " root");
			if (!error.empty())
				Logger::warning(error, __FILE__, __LINE__, __PRETTY_FUNCTION__);
			directives.setRootPath((*it)->value);
		}
		else
			Logger::warning("Unknown token in " + context + " root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in " + context + " root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " root directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateAutoindexDirective(const AST::ASTNode &directive, Directives &directives,
													std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " autoindex directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			if ((*it)->value == "on" || (*it)->value == "off")
				directives.setAutoIndex((*it)->value == "on");
			else
				Logger::warning("Invalid autoindex value: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else
		{
			Logger::warning("Unknown token in " + context + " autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in " + context + " autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",

							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " autoindex directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateCgiPathDirective(const AST::ASTNode &directive, Directives &directives,
												  std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " cgi path directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			// Historically this directive expected an executable file path (interpreter).
			// We now treat it as a BASE DIRECTORY that contains CGI scripts. Normal HTTP
			// URI resolution will append the portion of the request URI after the location
			// prefix to this directory when locating the actual script file.
			// Per user request: accept the path verbatim without strict verification.
			// (If the path does not exist or is not a directory, script resolution will
			// later fail gracefully returning 404.)
			std::string rawPath = (*it)->value;
			if (rawPath.empty())
			{
				Logger::warning("Empty cgi_path argument provided in " + context + " ignoring directive", __FILE__,
								__LINE__, __PRETTY_FUNCTION__);
				return;
			}
			// Normalize: remove trailing slashes for consistent concatenation
			while (rawPath.length() > 1 && rawPath[rawPath.length() - 1] == '/')
				rawPath.erase(rawPath.length() - 1);
			directives.setCgiPath(rawPath);
		}
		else
			Logger::warning("Unknown token in " + context + " cgi path directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in " + context + " cgi path directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " cgi path directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateClientMaxBodySizeDirective(const AST::ASTNode &directive, Directives &directives,
															std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " client max body size directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			double size = 0.0;
			if (!_parseSizeArgument((*it)->value, size))
				Logger::warning("Invalid client max body size in " + context + ": " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else
				directives.setClientMaxBodySize(size);
		}
		else
			Logger::warning("Unknown token in " + context + " client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in " + context + " client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " client max body size directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void _translateKeepAliveDirective(const AST::ASTNode &directive, Directives &directives, std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " keep alive directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			if ((*it)->value == "on" || (*it)->value == "off")
				directives.setKeepAlive((*it)->value == "on");
			else
				Logger::warning("Invalid keep alive value in " + context + ": " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else
			Logger::warning("Unknown token in " + context + " keep alive directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in " + context + " keep alive directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " keep alive directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void _translateRedirectDirective(const AST::ASTNode &directive, Directives &directives, std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " redirect directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		long long code = 0;
		std::string path = "";
		// Sanitize and verify return code
		if ((*it)->type == AST::ARG)
		{
			errno = 0;
			char *end;
			code = std::strtoll((*it)->value.c_str(), &end, 10);
			if (errno != 0 || end == (*it)->value.c_str() || *end != '\0' || code > 599 || code < 100)
			{
				Logger::warning("Invalid return code in " + context + " redirect directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}
		}
		else
		{
			Logger::warning("Unknown token in " + context + " redirect directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		++it;
		if (it == directive.children.end())
		{
			Logger::warning("No return path in " + context + " redirect directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			path = (*it)->value;
		}
		else
		{
			Logger::warning("Unknown token in location return directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		directives.setRedirect(std::make_pair(static_cast<int>(code), path));
		while (++it != directive.children.end())
			Logger::warning("Extra argument in " + context + " redirect directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " redirect directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void _translateIndexDirective(const AST::ASTNode &directive, Directives &directives, std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();

		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		for (; it != directive.children.end(); ++it)
		{
			if ((*it)->type == AST::ARG)
			{
				if (directives.hasIndex((*it)->value))
				{
					Logger::warning("Duplicate index in " + context + " index directive: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
					continue;
				}
				directives.insertIndex((*it)->value);
			}
			else
				Logger::warning("Unknown token in " + context + " index directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " index directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void _translateStatusPathDirective(const AST::ASTNode &directive, Directives &directives, std::string context)
{
	try
	{
		const std::vector<AST::ASTNode *> &children = directive.children;
		if (children.size() < 2)
		{
			Logger::warning("Error in " + context +
								" status_page directive requires at least one status code and a path line: " +
								StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		std::vector<int> codes;
		for (std::vector<AST::ASTNode *>::const_iterator it = children.begin(); it != children.end() - 1; ++it)
		{
			if ((*it)->type != AST::ARG)
			{
				Logger::warning("Unknown token in " + context + " status_page directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			char *end = NULL;
			double code = std::strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || code < 100 || code > 599)
			{
				Logger::warning("Invalid status code in " + context + " status_page directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			codes.push_back(static_cast<int>(code));
		}

		if (codes.empty())
		{
			Logger::warning("No valid status codes provided in " + context +
								" status_page directive line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		const AST::ASTNode *pathNode = children.back();
		if (pathNode->type != AST::ARG)
		{
			Logger::warning("Invalid path token in " + context +
								" status_page directive line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		const std::string &path = pathNode->value;
		if (path.empty())
		{
			Logger::warning("Empty path in " + context +
								" status_page directive line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if (StrUtils::hasConsecutiveDots(path))
			Logger::warning("Status page path contains consecutive dots: " + path +
								" line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		else if (StrUtils::hasControlCharacters(path))
			Logger::warning("Status page path contains control characters: " + path +
								" line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);

		directives.insertStatusPath(codes, path);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating error_page directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void _translateAllowedMethodsDirective(const AST::ASTNode &directive, Directives &directives, std::string context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();

		if (it == directive.children.end())
		{
			Logger::warning("No arguments in " + context + " allowed_methods directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else
		{
			for (; it != directive.children.end(); ++it)
			{
				if ((*it)->type == AST::ARG)
				{
					if (directives.hasAllowedMethod((*it)->value))
					{
						Logger::warning("Duplicate allowed method in " + context + " allowed_methods directive: " +
											(*it)->value + " line: " + StrUtils::toString<int>((*it)->line) +
											" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
										__FILE__, __LINE__, __PRETTY_FUNCTION__);
						continue;
					}
					directives.insertAllowedMethod((*it)->value);
				}
				else
					Logger::warning("Unknown token in " + context + " allowed_methods directive: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating " + context + " allowed_methods directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

/*
** --------------------------------- UTILITY ---------------------------------
*/

bool ConfigTranslator::_parseSizeArgument(const std::string &rawValue, double &sizeOut)
{
	if (rawValue.empty())
		return false;

	std::string numericPart = rawValue;
	double multiplier = 1.0;
	const char suffix = rawValue[rawValue.size() - 1];
	if (!std::isdigit(static_cast<unsigned char>(suffix)) && suffix != '.')
	{
		char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(suffix)));
		if (upper == 'K')
			multiplier = 1024.0;
		else if (upper == 'M')
			multiplier = 1024.0 * 1024.0;
		else if (upper == 'G')
			multiplier = 1024.0 * 1024.0 * 1024.0;
		else
			return false;
		numericPart = rawValue.substr(0, rawValue.size() - 1);
	}

	char *end = NULL;
	double parsed = std::strtod(numericPart.c_str(), &end);
	if (end == numericPart.c_str() || *end != '\0' || parsed < 0)
		return false;

	sizeOut = parsed * multiplier;
	return true;
}
