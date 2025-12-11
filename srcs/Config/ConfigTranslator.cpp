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
	return (_servers);
}

/*
** --------------------------------- TRANSLATION ---------------------------------
*/

// Main translation function
// Iterate through the AST and translate the server blocks
void ConfigTranslator::_translate(const AST::ASTNode &ast)
{
	Server server;

	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::SERVER)
		{
			server = _translateServer(**it);
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
	Logger::debug("Propagating server-level members to locations", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	for (TrieTree<Location>::iterator locIt = server.getLocations().begin(); locIt != server.getLocations().end();
		 ++locIt)
	{
		Location &location = *locIt;
		Logger::debug("Propagating to " + location.getLocationPath(), __FILE__, __LINE__, __PRETTY_FUNCTION__);
		if (server.hasRootPathDirective() && !location.hasRootPathDirective())
			location.setRootPath(*server.getRootPath());
		if (server.hasAutoIndexDirective() && !location.hasAutoIndexDirective())
			location.setAutoIndex(server.getAutoIndexValue());
		if (server.hasClientMaxBodySizeDirective() && !location.hasClientMaxBodySizeDirective())
			location.setClientMaxBodySize(server.getClientMaxBodySize());
		if (server.hasKeepAliveDirective() && !location.hasKeepAliveDirective())
			location.setKeepAlive(server.getKeepAliveValue());
		if (server.hasIndexDirective() && !location.hasIndexDirective())
			location.setIndexes(*server.getIndexes());
		if (server.hasStatusPathDirective() && !location.hasStatusPathDirective())
			location.setStatusPaths(*server.getStatusPaths());
		if (server.hasAllowedMethodsDirective() && !location.hasAllowedMethodsDirective())
			location.setAllowedMethods(*server.getAllowedMethods());
		if (location.getLocationType() == Directives::STATIC)
		{
			switch (server.getLocationType())
			{
			case Server::STATIC:
				break;
			case Server::REDIRECT:
				location.setRedirect(*server.getRedirect());
				break;
			case Server::CGI:
				location.setIsCgiPath(server.getisCgiPathValue());
				break;
			case Server::UPLOAD:
				location.setUploadPath(*server.getUploadPath());
				break;
			}
		}
	}
	Logger::debug("Inserting default root location if needed", __FILE__, __LINE__, __PRETTY_FUNCTION__);
	// Default location block, only created if root path was set at server level
	if (!server.getLocation("/") && server.wasModified() && server.hasRootPathDirective())
	{
		Location rootLocation(server.getRootPath()->empty() ? "/" : "/");
		if (server.hasRootPathDirective())
			rootLocation.setRootPath(*server.getRootPath());
		if (server.hasAutoIndexDirective())
			rootLocation.setAutoIndex(server.getAutoIndexValue());
		if (server.hasClientMaxBodySizeDirective())
			rootLocation.setClientMaxBodySize(server.getClientMaxBodySize());
		if (server.hasKeepAliveDirective())
			rootLocation.setKeepAlive(server.getKeepAliveValue());
		if (server.hasIndexDirective())
			rootLocation.setIndexes(*server.getIndexes());
		if (server.hasStatusPathDirective())
			rootLocation.setStatusPaths(*server.getStatusPaths());
		if (server.hasAllowedMethodsDirective())
			rootLocation.setAllowedMethods(*server.getAllowedMethods());
		switch (server.getLocationType())
		{
		case Server::STATIC:
			break;
		case Server::REDIRECT:
			rootLocation.setRedirect(*server.getRedirect());
			break;
		case Server::CGI:
			rootLocation.setIsCgiPath(server.getisCgiPathValue());
			break;
		case Server::UPLOAD:
			rootLocation.setUploadPath(*server.getUploadPath());
			break;
		}
		if (rootLocation.wasModified())
			server.insertLocation(rootLocation);
	}
	return (server);
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
			return (Logger::warning("No arguments in listen directive" + directive.value +
										" line: " + StrUtils::toString<int>(directive.line) +
										" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__));
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
										   Directives &directives, const std::string &context)
{
	if ((*directive)->value == "root")
		_translateRootPathDirective(**directive, directives, context);
	else if ((*directive)->value == "upload_path")
		_translateUploadPathDirective(**directive, directives, context);
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
												   const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "root", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "root", **it);

		std::string error = StrUtils::validateDirectoryPath((*it)->value, context + " root");
		if (!error.empty())
			Logger::warning(error, __FILE__, __LINE__, __PRETTY_FUNCTION__);
		directives.setRootPath((*it)->value);
		_warnExtraArgs(it, directive.children.end(), context, "root");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "root", directive, e);
	}
}

void ConfigTranslator::_translateAutoindexDirective(const AST::ASTNode &directive, Directives &directives,
													const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "autoindex", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "autoindex", **it);

		if ((*it)->value == "on")
			directives.setAutoIndex(true);
		else if ((*it)->value == "off")
			directives.setAutoIndex(false);
		else
			Logger::warning("Invalid autoindex value: " + (*it)->value + _nodeLocation(*it) + " skipping...", __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
		_warnExtraArgs(it, directive.children.end(), context, "autoindex");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "autoindex", directive, e);
	}
}

void ConfigTranslator::_translateUploadPathDirective(const AST::ASTNode &directive, Directives &directives,
													 const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "upload_path", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "upload_path", **it);

		std::string error = StrUtils::validateDirectoryPath((*it)->value, context + " upload_path");
		if (!error.empty())
			Logger::warning("Invalid upload_path value: " + (*it)->value + " error: " + error + _nodeLocation(*it) +
								" skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		else
			directives.setUploadPath((*it)->value);
		_warnExtraArgs(it, directive.children.end(), context, "upload_path");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "upload_path", directive, e);
	}
}

void ConfigTranslator::_translateCgiPathDirective(const AST::ASTNode &directive, Directives &directives,
												  const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "cgi_path", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "cgi_path", **it);

		if ((*it)->value == "true")
			directives.setIsCgiPath(true);
		else if ((*it)->value == "false")
			directives.setIsCgiPath(false);
		else
		{
			Logger::warning("Invalid cgi_path value: " + (*it)->value + _nodeLocation(*it) + " skipping...", __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
			return;
		}
		_warnExtraArgs(it, directive.children.end(), context, "cgi_path");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "cgi_path", directive, e);
	}
}

void ConfigTranslator::_translateClientMaxBodySizeDirective(const AST::ASTNode &directive, Directives &directives,
															const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "client_max_body_size", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "client_max_body_size", **it);

		double size = 0.0;
		if (!_parseSizeArgument((*it)->value, size))
			Logger::warning("Invalid client_max_body_size value: " + (*it)->value + _nodeLocation(*it) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		else
			directives.setClientMaxBodySize(size);
		_warnExtraArgs(it, directive.children.end(), context, "client_max_body_size");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "client_max_body_size", directive, e);
	}
}

void ConfigTranslator::_translateKeepAliveDirective(const AST::ASTNode &directive, Directives &directives,
													const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "keep_alive", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "keep_alive", **it);

		if ((*it)->value == "on" || (*it)->value == "off")
			directives.setKeepAlive((*it)->value == "on");
		else
			Logger::warning("Invalid keep_alive value: " + (*it)->value + _nodeLocation(*it) + " skipping...", __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
		_warnExtraArgs(it, directive.children.end(), context, "keep_alive");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "keep_alive", directive, e);
	}
}

void ConfigTranslator::_translateRedirectDirective(const AST::ASTNode &directive, Directives &directives,
												   const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "redirect", directive);
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "redirect", **it);

		// Parse status code
		char *end;
		errno = 0;
		long long code = std::strtoll((*it)->value.c_str(), &end, 10);
		if (errno != 0 || end == (*it)->value.c_str() || *end != '\0' || code > 400 || code < 299)
		{
			Logger::warning("Invalid redirect code: " + (*it)->value + _nodeLocation(*it) + " skipping...", __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
			return;
		}

		// Parse redirect path
		++it;
		if (it == directive.children.end())
		{
			Logger::warning("Missing redirect path in " + context + _nodeLocation(directive) + " skipping...", __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type != AST::ARG)
			return _warnUnknownToken(context, "redirect", **it);

		directives.setRedirect(std::make_pair(static_cast<int>(code), (*it)->value));
		_warnExtraArgs(it, directive.children.end(), context, "redirect");
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "redirect", directive, e);
	}
}

void ConfigTranslator::_translateIndexDirective(const AST::ASTNode &directive, Directives &directives,
												const std::string &context)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
			return _warnNoArgs(context, "index", directive);

		for (; it != directive.children.end(); ++it)
		{
			if ((*it)->type != AST::ARG)
			{
				_warnUnknownToken(context, "index", **it);
				continue;
			}
			if (directives.hasIndex((*it)->value))
			{
				Logger::warning("Duplicate index: " + (*it)->value + _nodeLocation(*it) + " skipping...", __FILE__,
								__LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			directives.insertIndex((*it)->value);
		}
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "index", directive, e);
	}
}

void ConfigTranslator::_translateStatusPathDirective(const AST::ASTNode &directive, Directives &directives,
													 const std::string &context)
{
	char *end;
	double code;

	try
	{
		const std::vector<AST::ASTNode *> &children = directive.children;
		if (children.size() < 2)
		{
			Logger::warning("Error in " + context +
								" status_page directive requires at least one status code and a path" +
								_nodeLocation(directive) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		std::vector<int> codes;
		for (std::vector<AST::ASTNode *>::const_iterator it = children.begin(); it != children.end() - 1; ++it)
		{
			if ((*it)->type != AST::ARG)
			{
				_warnUnknownToken(context, "status_page", **it);
				continue;
			}
			end = NULL;
			code = std::strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || code < 100 || code > 599)
			{
				Logger::warning("Invalid status code in " + context + " status_page directive: " + (*it)->value +
									_nodeLocation(*it) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			codes.push_back(static_cast<int>(code));
		}
		if (codes.empty())
		{
			Logger::warning("No valid status codes provided in " + context + " status_page directive" +
								_nodeLocation(directive) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		const AST::ASTNode *pathNode = children.back();
		if (pathNode->type != AST::ARG)
		{
			Logger::warning("Invalid path token in " + context + " status_page directive" + _nodeLocation(pathNode) +
								" skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		const std::string &path = pathNode->value;
		if (path.empty())
		{
			Logger::warning("Empty path in " + context + " status_page directive" + _nodeLocation(pathNode) +
								" skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if (StrUtils::hasConsecutiveDots(path))
			Logger::warning("Status page path contains consecutive dots: " + path + _nodeLocation(pathNode), __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
		else if (StrUtils::hasControlCharacters(path))
			Logger::warning("Status page path contains control characters: " + path + _nodeLocation(pathNode), __FILE__,
							__LINE__, __PRETTY_FUNCTION__);
		directives.insertStatusPath(codes, path);
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "status_page", directive, e);
	}
}

void ConfigTranslator::_translateAllowedMethodsDirective(const AST::ASTNode &directive, Directives &directives,
														 const std::string &context)
{
	try
	{
		if (directive.children.empty())
		{
			_warnNoArgs(context, "allowed_methods", directive);
			return;
		}
		for (std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
			 it != directive.children.end(); ++it)
		{
			if ((*it)->type != AST::ARG)
			{
				_warnUnknownToken(context, "allowed_methods", **it);
				continue;
			}
			if (directives.hasAllowedMethod((*it)->value))
			{
				Logger::warning("Duplicate allowed method in " + context +
									" allowed_methods directive: " + (*it)->value + _nodeLocation(*it) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			directives.insertAllowedMethod((*it)->value);
		}
	}
	catch (const std::exception &e)
	{
		_errorTranslating(context, "allowed_methods", directive, e);
	}
}

/*
** --------------------------------- UTILITY ---------------------------------
*/

// Helper: Format node location for logging
std::string ConfigTranslator::_nodeLocation(const AST::ASTNode &node)
{
	return " line: " + StrUtils::toString<int>(node.line) + " column: " + StrUtils::toString<int>(node.column);
}

std::string ConfigTranslator::_nodeLocation(const AST::ASTNode *node)
{
	return _nodeLocation(*node);
}

// Helper: Warn about missing arguments
void ConfigTranslator::_warnNoArgs(const std::string &context, const std::string &directiveName,
								   const AST::ASTNode &node)
{
	Logger::warning("No arguments in " + context + " " + directiveName + " directive" + _nodeLocation(node) +
						" skipping...",
					__FILE__, __LINE__, __PRETTY_FUNCTION__);
}

// Helper: Warn about unknown token type
void ConfigTranslator::_warnUnknownToken(const std::string &context, const std::string &directiveName,
										 const AST::ASTNode &node)
{
	Logger::warning("Unknown token in " + context + " " + directiveName + " directive: " + node.value +
						_nodeLocation(node) + " skipping...",
					__FILE__, __LINE__, __PRETTY_FUNCTION__);
}

// Helper: Warn about extra arguments
void ConfigTranslator::_warnExtraArgs(std::vector<AST::ASTNode *>::const_iterator it,
									  std::vector<AST::ASTNode *>::const_iterator end, const std::string &context,
									  const std::string &directiveName)
{
	while (++it != end)
	{
		Logger::warning("Extra argument in " + context + " " + directiveName + " directive: " + (*it)->value +
							_nodeLocation(*it) + " skipping...",
						__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// Helper: Log translation error
void ConfigTranslator::_errorTranslating(const std::string &context, const std::string &directiveName,
										 const AST::ASTNode &node, const std::exception &e)
{
	Logger::error("Error translating " + context + " " + directiveName + " directive: " + std::string(e.what()) +
					  _nodeLocation(node) + " skipping...",
				  __FILE__, __LINE__, __PRETTY_FUNCTION__);
}

bool ConfigTranslator::_parseSizeArgument(const std::string &rawValue, double &sizeOut)
{
	double multiplier;
	const char suffix = rawValue[rawValue.size() - 1];
	char upper;
	char *end;
	double parsed;

	if (rawValue.empty())
		return (false);
	std::string numericPart = rawValue;
	multiplier = 1.0;
	if (!std::isdigit(static_cast<unsigned char>(suffix)) && suffix != '.')
	{
		upper = static_cast<char>(std::toupper(static_cast<unsigned char>(suffix)));
		if (upper == 'K')
			multiplier = 1024.0;
		else if (upper == 'M')
			multiplier = 1024.0 * 1024.0;
		else if (upper == 'G')
			multiplier = 1024.0 * 1024.0 * 1024.0;
		else
			return (false);
		numericPart = rawValue.substr(0, rawValue.size() - 1);
	}
	end = NULL;
	parsed = std::strtod(numericPart.c_str(), &end);
	if (end == numericPart.c_str() || *end != '\0' || parsed < 0)
		return (false);
	sizeOut = parsed * multiplier;
	return (true);
}
