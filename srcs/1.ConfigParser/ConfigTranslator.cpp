#include "../../includes/ConfigParser/ConfigTranslator.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/HTTP.hpp"
#include <cctype>
#include <cstdlib>
#include <vector>

namespace
{
bool parseSizeArgument(const std::string &rawValue, double &sizeOut)
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

} // namespace

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigTranslator::ConfigTranslator(const AST::ASTNode &ast)
{
	_translate(ast);
}

/*
** ------------------------------- DESTRUCTOR --------------------------------
*/

ConfigTranslator::~ConfigTranslator()
{
}

/*
** ------------------------------- Accessors --------------------------------
*/

const std::vector<Server> &ConfigTranslator::getServers() const
{
	return _servers;
}

// Iterate through the AST and translate the server blocks
void ConfigTranslator::_translate(const AST::ASTNode &ast)
{
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::SERVER)
		{
			Server server = _translateServer(**it);
			if (!server.isModified())
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
			else if ((*it)->value == "root")
				_translateServerRoot(**it, server);
			else if ((*it)->value == "index")
				_translateServerIndex(**it, server);
			else if ((*it)->value == "autoindex")
				_translateServerAutoindex(**it, server);
			else if ((*it)->value == "client_max_body_size")
				_translateServerClientMaxBodySize(**it, server);
			else if ((*it)->value == "error_pages")
				_translateServerErrorPages(**it, server);
			else
				Logger::warning("Unknown directive in server block: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else if ((*it)->type == AST::LOCATION)
		{
			Location location((*it)->value);
			Logger::debug("Processing location block: " + (*it)->value, __FILE__, __LINE__, __PRETTY_FUNCTION__);
			_translateLocation(**it, location);
			Logger::debug("Location modified: " + std::string(location.wasModified() ? "true" : "false"), __FILE__,
						  __LINE__, __PRETTY_FUNCTION__);
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
		if (!locIt->hasRootDirective() && !server.hasRootPathDirective())
			locIt->setRoot(server.getRootPath());
		if (!locIt->hasIndexesDirective() && !server.hasIndexDirective())
			locIt->setIndexes(server.getIndexes());
		if (!locIt->hasAllowedMethodsDirective() && !server.hasAllowedMethodsDirective())
			locIt->setAllowedMethods(server.getAllowedMethods());
	}
	return server;
}

/*
** --------------------------------- SERVER SPECIFIC HELPERS ---------------------------------
*/

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
		return;
	}
}

void ConfigTranslator::_translateServerRoot(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in server root directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			std::string error = StrUtils::validateDirectoryPath((*it)->value, "server root");
			if (!error.empty())
				Logger::warning(error, __FILE__, __LINE__, __PRETTY_FUNCTION__);
			server.setRoot((*it)->value);
		}
		else
			Logger::warning("Unknown token in server root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in server root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating server root directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
// Translate index directives into server members
void ConfigTranslator::_translateServerIndex(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();

		if (it == directive.children.end())
		{
			Logger::warning("No arguments in server index directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		for (; it != directive.children.end(); ++it)
		{
			if ((*it)->type == AST::ARG)
			{
				if (server.hasIndex((*it)->value))
				{
					Logger::warning("Duplicate index in server index directive: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
					continue;
				}
				server.insertIndex((*it)->value);
			}
			else
				Logger::warning("Unknown token in server index directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating server index directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// Translate autoindex directives into server members
void ConfigTranslator::_translateServerAutoindex(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in autoindex directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			if ((*it)->value == "on" || (*it)->value == "off")
				server.setAutoindex((*it)->value == "on");
			else
				Logger::warning("Invalid autoindex value: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else
		{
			Logger::warning("Unknown token in autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",

							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating autoindex directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// Translate client max body size directives into server members
void ConfigTranslator::_translateServerClientMaxBodySize(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in client max body size directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			double size = 0.0;
			if (!parseSizeArgument((*it)->value, size))
				Logger::warning("Invalid client max body size: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else
				server.setClientMaxBodySize(size);
		}
		else
			Logger::warning("Unknown token in client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating client max body size directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return;
	}
}

// Translate status page directives into server members
void ConfigTranslator::_translateServerErrorPages(const AST::ASTNode &directive, Server &server)
{
	try
	{
		const std::vector<AST::ASTNode *> &children = directive.children;
		if (children.size() < 2)
		{
			Logger::warning("error_page directive requires at least one status code and a path line: " +
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
				Logger::warning("Unknown token in error_page directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			char *end = NULL;
			double code = std::strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || code < 100 || code > 599)
			{
				Logger::warning("Invalid status code in error_page directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				continue;
			}
			codes.push_back(static_cast<int>(code));
		}

		if (codes.empty())
		{
			Logger::warning("No valid status codes provided in error_page directive line: " +
								StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		const AST::ASTNode *pathNode = children.back();
		if (pathNode->type != AST::ARG)
		{
			Logger::warning(
				"Invalid path token in error_page directive line: " + StrUtils::toString<int>(pathNode->line) +
					" column: " + StrUtils::toString<int>(pathNode->column) + " skipping...",
				__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		const std::string &path = pathNode->value;
		if (path.empty())
		{
			Logger::warning("Empty path in error_page directive line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if (!StrUtils::isAbsolutePath(path))
			Logger::warning("Status page path is not absolute: " + path +
								" line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		else if (StrUtils::hasConsecutiveDots(path))
			Logger::warning("Status page path contains consecutive dots: " + path +
								" line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		else if (StrUtils::hasControlCharacters(path))
			Logger::warning("Status page path contains control characters: " + path +
								" line: " + StrUtils::toString<int>(pathNode->line) +
								" column: " + StrUtils::toString<int>(pathNode->column),
							__FILE__, __LINE__, __PRETTY_FUNCTION__);

		server.insertStatusPage(path, codes);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating error_page directive: " + std::string(e.what()) +
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
		Logger::debug("Location has " + StrUtils::toString<int>(location_node.children.size()) + " children", __FILE__,
					  __LINE__, __PRETTY_FUNCTION__);
		for (std::vector<AST::ASTNode *>::const_iterator it = location_node.children.begin();
			 it != location_node.children.end(); ++it)
		{
			Logger::debug("Processing child: type=" + StrUtils::toString<int>((*it)->type) + ", value=" + (*it)->value,
						  __FILE__, __LINE__, __PRETTY_FUNCTION__);
			if ((*it)->type == AST::DIRECTIVE)
			{
				if ((*it)->value == "root")
					_translateLocationRoot(**it, location);
				else if ((*it)->value == "allowed_methods")
					_translateLocationAllowedMethods(**it, location);
				else if ((*it)->value == "error_page")
					_translateLocationErrorPages(**it, location);
				else if ((*it)->value == "return")
					_translateLocationRedirect(**it, location);
				else if ((*it)->value == "autoindex")
					_translateLocationAutoindex(**it, location);
				else if ((*it)->value == "index")
					_translateLocationIndex(**it, location);
				else if ((*it)->value == "cgi_path")
					_translateLocationCgiPath(**it, location);
				else if ((*it)->value == "client_max_body_size")
					_translateLocationClientMaxBodySize(**it, location);
				else
					Logger::warning("Unknown directive in location block: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
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

void ConfigTranslator::_translateLocationClientMaxBodySize(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location client max body size directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			double size = 0.0;
			if (!parseSizeArgument((*it)->value, size))
			{
				Logger::warning("Invalid location client max body size: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}
			location.setClientMaxBodySize(size);
		}
		else
		{
			Logger::warning("Unknown token in location client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in location client max body size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location client max body size directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// void ConfigTranslator::_translateLocationCgiParam(const AST::ASTNode &directive, Location &location)
// {
// 	try
// 	{
// 		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
// 		if (it == directive.children.end())
// 		{
// 			Logger::warning("No arguments in location cgi param directive: " + directive.value +
// 								" line: " + StrUtils::toString<int>(directive.line) +
// 								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
// 							__FILE__, __LINE__, __PRETTY_FUNCTION__);
// 			return;
// 		}
// 		if ((*it)->type != AST::ARG)
// 		{
// 			Logger::warning("Invalid key token in location cgi param directive: " + (*it)->value +
// 								" line: " + StrUtils::toString<int>((*it)->line) +
// 								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
// 							__FILE__, __LINE__, __PRETTY_FUNCTION__);
// 			return;
// 		}
// 		const std::string key = (*it)->value;
// 		++it;
// 		if (it == directive.children.end() || (*it)->type != AST::ARG)
// 		{
// 			Logger::warning("Missing value in location cgi param directive: " + directive.value +
// 								" line: " + StrUtils::toString<int>(directive.line) +
// 								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
// 							__FILE__, __LINE__, __PRETTY_FUNCTION__);
// 			return;
// 		}
// 		const std::string value = (*it)->value;
// 		location.setCgiParam(key, value);
// 		while (++it != directive.children.end())
// 		{
// 			Logger::warning("Extra argument in location cgi param directive: " + (*it)->value +
// 								" line: " + StrUtils::toString<int>((*it)->line) +
// 								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
// 							__FILE__, __LINE__, __PRETTY_FUNCTION__);
// 		}
// 	}
// 	catch (const std::exception &e)
// 	{
// 		Logger::error("Error translating location cgi param directive: " + std::string(e.what()) +
// 						  " line: " + StrUtils::toString<int>(directive.line) +
// 						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
// 					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
// 	}
// }

void ConfigTranslator::_translateLocationRoot(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location root directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			std::string error = StrUtils::validateDirectoryPath((*it)->value, "location root");
			if (!error.empty())
				Logger::warning(error + " line: " + StrUtils::toString<int>(directive.line) +
									" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else
				location.setRoot((*it)->value);
		}
		else
			Logger::warning("Unknown token in location root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
			Logger::warning("Extra argument in location root directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location root directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		return;
	}
}

void ConfigTranslator::_translateLocationAllowedMethods(const AST::ASTNode &directive, Location &location)
{
	try
	{
		Logger::debug("Processing allowed_methods directive with " +
						  StrUtils::toString<int>(directive.children.size()) + " children",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location allowed methods directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		for (; it != directive.children.end(); ++it)
		{
			Logger::debug("Processing allowed method: " + (*it)->value, __FILE__, __LINE__, __PRETTY_FUNCTION__);
			if (location.hasAllowedMethod((*it)->value))
				Logger::warning("Duplicate allowed method: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else if (!HTTP::isSupportedMethod((*it)->value))
				Logger::warning("Unsupported allowed method: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			location.insertAllowedMethod((*it)->value);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location allowed methods directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateLocationRedirect(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location return directive: " + directive.value +
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
				Logger::warning("Invalid return code: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}
		}
		else
		{
			Logger::warning("Unknown token in location return directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		++it;
		if (it == directive.children.end())
		{
			Logger::warning("No return path in location return directive: " + (*it)->value +
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
		location.setRedirect(std::make_pair(static_cast<int>(code), path));
		while (++it != directive.children.end())
			Logger::warning("Extra argument in location return directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location return directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateLocationAutoindex(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location autoindex directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		if ((*it)->type == AST::ARG)
		{
			if ((*it)->value == "on" || (*it)->value == "off")
				location.setAutoIndex((*it)->value == "on" ? true : false);
			else
				Logger::warning("Invalid autoindex value: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else
			Logger::warning("Unknown token in location autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
			Logger::warning("Extra argument in location autoindex directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location autoindex directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateLocationIndex(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location index directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		for (; it != directive.children.end(); ++it)
		{
			if ((*it)->type == AST::ARG)
			{
				if ((*it)->value.empty())
				{
					Logger::warning("Empty index path: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
					return;
				}
				if (location.hasIndex((*it)->value))
					Logger::warning("Duplicate index path: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
				else
					location.insertIndex((*it)->value);
			}
			else
				Logger::warning("Unknown token in location index directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location index directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateLocationCgiPath(const AST::ASTNode &directive, Location &location)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location cgi path directive: " + directive.value +
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
				Logger::warning("Empty cgi_path argument provided; ignoring directive", __FILE__, __LINE__,
								__PRETTY_FUNCTION__);
				return;
			}
			// Normalize: remove trailing slashes for consistent concatenation
			while (rawPath.length() > 1 && rawPath[rawPath.length() - 1] == '/')
				rawPath.erase(rawPath.length() - 1);
			location.setCgiPath(rawPath);
		}
		else
			Logger::warning("Unknown token in location cgi path directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);

		// Ignore any extra args silently (previously logged as warnings)
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location cgi path directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

void ConfigTranslator::_translateLocationErrorPages(const AST::ASTNode &directive, Location &location)
{
	try
	{
		if (directive.children.empty())
		{
			Logger::warning("Empty location error pages directive at line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}

		// Get the first argument (status code)
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if ((*it)->type == AST::ARG)
		{
			int statusCode = StrUtils::fromString<int>((*it)->value);
			if (statusCode < 100 || statusCode > 599)
			{
				Logger::warning("Invalid status code in location error pages directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}

			// Get the second argument (file path)
			++it;
			if (it != directive.children.end() && (*it)->type == AST::ARG)
			{
				std::string filePath = (*it)->value;
				std::vector<int> codes;
				codes.push_back(statusCode);
				location.insertStatusPage(codes, filePath);
			}
			else
			{
				Logger::warning("Missing file path in location error pages directive at line: " +
									StrUtils::toString<int>(directive.line) +
									" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			}
		}
		else
		{
			Logger::warning(
				"Invalid location error pages directive at line: " + StrUtils::toString<int>(directive.line) +
					" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
				__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location error pages directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}
