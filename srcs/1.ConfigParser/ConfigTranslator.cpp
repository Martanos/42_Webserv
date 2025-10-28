#include "../../includes/ConfigParser/ConfigTranslator.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Global/StrUtils.hpp"
#include "../../includes/HTTP/Constants.hpp"
#include <vector>

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
			else if ((*it)->value == "client_max_headers_size")
				_translateServerClientMaxHeadersSize(**it, server);
			else if ((*it)->value == "client_max_uri_size")
				_translateServerClientMaxUriSize(**it, server);
			else if ((*it)->value == "status_pages")
				_translateServerStatusPage(**it, server);
			else
				Logger::warning("Unknown directive in server block: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		else if ((*it)->type == AST::LOCATION)
		{
			Location location((*it)->value);
			if (location.hasModified())
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
		{
			Logger::warning("No arguments in listen directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
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
				Logger::warning(error);
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
		while (++it != directive.children.end())
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
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
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

// Translate client max headers size directives into server members
void ConfigTranslator::_translateServerClientMaxHeadersSize(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in client max headers size directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
				throw std::runtime_error("Invalid client max headers size: " + (*it)->value);
			server.setClientMaxHeadersSize(size);
		}
		else
			Logger::warning("Unknown token in client max headers size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
			Logger::warning("Extra argument in client max headers size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::warning("Error translating client max headers size directive: " + std::string(e.what()) +
							" line: " + StrUtils::toString<int>(directive.line) +
							" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
						__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// Translate client max uri size directives into server members
void ConfigTranslator::_translateServerClientMaxUriSize(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in client max uri size directive" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
				Logger::warning("Invalid client max uri size: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else
				server.setClientMaxUriSize(size);
		}
		else
			Logger::warning("Unknown token in client max uri size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		while (++it != directive.children.end())
		{
			Logger::warning("Extra argument in client max uri size directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating client max uri size directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}

// Translate status page directives into server members
void ConfigTranslator::_translateServerStatusPage(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<int> codes;
		std::string path;
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		// Collect status page codes
		while (++it != directive.children.end() - 1)
		{
			if ((*it)->type == AST::ARG)
			{
				char *end;
				double code = strtod((*it)->value.c_str(), &end);
				if (end == (*it)->value.c_str() || *end != '\0' || code > 599 || code < 100)
					Logger::warning("Invalid status page code: " + (*it)->value +
										" line: " + StrUtils::toString<int>((*it)->line) +
										" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
									__FILE__, __LINE__, __PRETTY_FUNCTION__);
				else
					codes.push_back(static_cast<int>(code));
			}
			else
				Logger::warning("Unknown token in status page directive: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
		}
		if (codes.empty())
		{
			Logger::warning("No valid status page codes" + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		else if ((*it)->type == AST::ARG)
		{
			if ((*it)->value.empty())
			{
				Logger::warning("Empty status page path" + directive.value +
									" line: " + StrUtils::toString<int>(directive.line) +
									" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}
			else if (!StrUtils::isAbsolutePath((*it)->value))
				Logger::warning("Status page path is not absolute: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else if (StrUtils::hasConsecutiveDots((*it)->value))
				Logger::warning("Status page path contains consecutive dots: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			else if (StrUtils::hasControlCharacters((*it)->value))
				Logger::warning("Status page path contains control characters: " + (*it)->value +
									" line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column),
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			server.insertStatusPage((*it)->value, codes);
		}
		else
			Logger::warning("Unknown token in status page directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating status page directive: " + std::string(e.what()) +
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
			if ((*it)->type == AST::DIRECTIVE)
			{
				if ((*it)->value == "root")
					_translateLocationRoot(**it, location);
				else if ((*it)->value == "allowed_methods")
					_translateLocationAllowedMethods(**it, location);
				else if ((*it)->value == "status_page")
					_translateLocationStatusPage(**it, location);
				else if ((*it)->value == "return")
					_translateLocationRedirect(**it, location);
				else if ((*it)->value == "autoindex")
					_translateLocationAutoindex(**it, location);
				else if ((*it)->value == "index")
					_translateLocationIndex(**it, location);
				else if ((*it)->value == "cgi_path")
					_translateLocationCgiPath(**it, location);
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
		std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
		if (it == directive.children.end())
		{
			Logger::warning("No arguments in location allowed methods directive: " + directive.value +
								" line: " + StrUtils::toString<int>(directive.line) +
								" column: " + StrUtils::toString<int>(directive.column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
			return;
		}
		while (++it != directive.children.end())
		{
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
		while (++it != directive.children.end())
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
			if ((*it)->value.empty())
			{
				Logger::warning("Empty cgi path: " + (*it)->value + " line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
				return;
			}
			std::string error = StrUtils::validateFilePath((*it)->value, "location cgi path");
			if (!error.empty())
				Logger::warning(error + " line: " + StrUtils::toString<int>((*it)->line) +
									" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
								__FILE__, __LINE__, __PRETTY_FUNCTION__);
			location.setCgiPath((*it)->value);
		}
		else
			Logger::warning("Unknown token in location cgi path directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);

		while (++it != directive.children.end())
			Logger::warning("Extra argument in location cgi path directive: " + (*it)->value +
								" line: " + StrUtils::toString<int>((*it)->line) +
								" column: " + StrUtils::toString<int>((*it)->column) + " skipping...",
							__FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
	catch (const std::exception &e)
	{
		Logger::error("Error translating location cgi path directive: " + std::string(e.what()) +
						  " line: " + StrUtils::toString<int>(directive.line) +
						  " column: " + StrUtils::toString<int>(directive.column) + " skipping...",
					  __FILE__, __LINE__, __PRETTY_FUNCTION__);
	}
}