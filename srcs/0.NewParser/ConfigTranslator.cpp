#include "../../includes/ConfigParser/ConfigTranslator.hpp"
#include "../../includes/Global/StringUtils.hpp"
#include <vector>

ConfigTranslator::ConfigTranslator(const AST::ASTNode &ast)
{
	_translate(ast);
}

ConfigTranslator::~ConfigTranslator()
{
}

const std::vector<Server> &ConfigTranslator::getServers() const
{
	return _servers;
}

void ConfigTranslator::_translate(const AST::ASTNode &ast)
{
	// Translate Server Directives first
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::SERVER)
		{
			Server server = _translateServer(**it);
			if (server.isModified())
			{
				_servers.push_back(server);
			}
		}
	}
}

Server ConfigTranslator::_translateServer(const AST::ASTNode &ast)
{
	Server server;
	// Traverse the server block and translate recognizable members
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::DIRECTIVE)
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
			else if ((*it)->value == "error_page")
				_translateServerStatusPage(**it, server);
			else
			{
				std::stringstream ss;
				ss << "Unknown directive in server block: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << std::endl;
				Logger::warning(ss.str());
			}
		}
		else if ((*it)->type == AST::NodeType::LOCATION)
		{
			Location location = _translateLocation(**it);
			if (location.isModified())
			{
				server.insertLocation(location);
			}
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in server block: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << std::endl;
			Logger::warning(ss.str());
		}
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
			if ((*it)->type == AST::NodeType::ARG)
			{
				if (server.getServerName((*it)->value) == NULL)
				{
					server.insertServerName((*it)->value);
				}
				else
				{
					std::stringstream ss;
					ss << "Duplicate server name detected : " << (*it)->value << " line: " << (*it)->line
					   << " column: " << (*it)->column << " skipping..." << std::endl;
					Logger::warning(ss.str());
				}
			}
			else
			{
				std::stringstream ss;
				ss << "Unknown token in server name block: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << std::endl;
				Logger::warning(ss.str());
			}
		}
		catch (const std::exception &e)
		{
			std::stringstream ss;
			ss << "Error translating server name: " << e.what() << " line: " << directive.line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
}

// Translate listen directives into server members
void ConfigTranslator::_translateListen(const AST::ASTNode &directive, Server &server)
{
	std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
	if (it == directive.children.end())
	{
		std::stringstream ss;
		ss << "No arguments in listen directive: " << directive.value << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	try
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			Socket socket((*it)->value);
			server.insertSocket(socket);
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in listen directive: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating listen directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	// Warning for extra arguments
	while (++it != directive.children.end())
	{
		std::stringstream ss;
		ss << "Extra argument in listen directive: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << " skipping..." << std::endl;
		Logger::warning(ss.str());
	}
}

// Translate index directives into server members
void ConfigTranslator::_translateServerIndex(const AST::ASTNode &directive, Server &server)
{
	for (std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin(); it != directive.children.end();
		 ++it)
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			if ((*it)->value.empty())
			{
				std::stringstream ss;
				ss << "Empty index name: " << (*it)->value << " line: " << (*it)->line << " column: " << (*it)->column
				   << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			// No absolute paths
			if ((*it)->value[0] == '/')
			{
				std::stringstream ss;
				ss << "Absolute path in index name: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}

			if ((*it)->value.find("..") != std::string::npos)
			{
				std::stringstream ss;
				ss << "Traversal in index name: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}

			// No control chars
			for (size_t i = 0; i < (*it)->value.size(); ++i)
			{
				unsigned char c = (*it)->value[i];
				if (c < 0x20 || c == 0x7F)
				{
					std::stringstream ss;
					ss << "Control character in index name: " << (*it)->value << " line: " << (*it)->line
					   << " column: " << (*it)->column << " skipping..." << std::endl;
					Logger::warning(ss.str());
					return;
				}
			}

			server.insertIndex((*it)->value);
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in server index block: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << std::endl;
			Logger::warning(ss.str());
		}
	}
}

// Translate autoindex directives into server members
void ConfigTranslator::_translateServerAutoindex(const AST::ASTNode &directive, Server &server)
{
	std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
	if (it == directive.children.end())
	{
		std::stringstream ss;
		ss << "No arguments in autoindex directive: " << directive.value << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	try
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			server.setAutoindex((*it)->value == "on");
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in autoindex directive: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating autoindex directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	while (++it != directive.children.end())
	{
		std::stringstream ss;
		ss << "Extra argument in autoindex directive: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << " skipping..." << std::endl;
		Logger::warning(ss.str());
	}
}

// Translate client max body size directives into server members
void ConfigTranslator::_translateServerClientMaxBodySize(const AST::ASTNode &directive, Server &server)
{
	std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
	if (it == directive.children.end())
	{
		std::stringstream ss;
		ss << "No arguments in client max body size directive: " << directive.value << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	try
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
			{
				std::stringstream ss;
				ss << "Invalid client max body size: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			server.setClientMaxBodySize(size);
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in client max body size directive: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating client max body size directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	while (++it != directive.children.end())
	{
		std::stringstream ss;
		ss << "Extra argument in client max body size directive: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << " skipping..." << std::endl;
		Logger::warning(ss.str());
	}
}

// Translate client max headers size directives into server members
void ConfigTranslator::_translateServerClientMaxHeadersSize(const AST::ASTNode &directive, Server &server)
{
	std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
	if (it == directive.children.end())
	{
		std::stringstream ss;
		ss << "No arguments in client max headers size directive: " << directive.value << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	try
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
			{
				std::stringstream ss;
				ss << "Invalid client max headers size: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			server.setClientMaxHeadersSize(size);
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in client max headers size directive: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating client max headers size directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	while (++it != directive.children.end())
	{
		std::stringstream ss;
		ss << "Extra argument in client max headers size directive: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << " skipping..." << std::endl;
		Logger::warning(ss.str());
	}
}

// Translate client max uri size directives into server members
void ConfigTranslator::_translateServerClientMaxUriSize(const AST::ASTNode &directive, Server &server)
{
	std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
	if (it == directive.children.end())
	{
		std::stringstream ss;
		ss << "No arguments in client max uri size directive: " << directive.value << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	try
	{
		if ((*it)->type == AST::NodeType::ARG)
		{
			char *end;
			double size = strtod((*it)->value.c_str(), &end);
			if (end == (*it)->value.c_str() || *end != '\0' || size < 0)
			{
				std::stringstream ss;
				ss << "Invalid client max uri size: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			server.setClientMaxUriSize(size);
		}
		else
		{
			std::stringstream ss;
			ss << "Unknown token in client max uri size directive: " << (*it)->value << " line: " << (*it)->line
			   << " column: " << (*it)->column << " skipping..." << std::endl;
			Logger::warning(ss.str());
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating client max uri size directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
	while (++it != directive.children.end())
	{
		std::stringstream ss;
		ss << "Extra argument in client max uri size directive: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << " skipping..." << std::endl;
		Logger::warning(ss.str());
	}
}

// Translate status page directives into server members
void ConfigTranslator::_translateServerStatusPage(const AST::ASTNode &directive, Server &server)
{
	try
	{
		std::vector<int> codes;
		std::string path;

		// Collect status page codes
		for (std::vector<AST::ASTNode *>::const_iterator it = directive.children.begin();
			 it != directive.children.end() - 1; ++it)
		{
			if ((*it)->type == AST::NodeType::ARG)
			{
				char *end;
				double code = strtod((*it)->value.c_str(), &end);
				if (end == (*it)->value.c_str() || *end != '\0' || code > 599 || code < 100)
				{
					std::stringstream ss;
					ss << "Invalid status page code: " << (*it)->value << " line: " << (*it)->line
					   << " column: " << (*it)->column << " skipping..." << std::endl;
					Logger::warning(ss.str());
					return;
				}
				else
					codes.insert(static_cast<int>(code));
			}
			else
			{
				std::stringstream ss;
				ss << "Unknown token in status page directive: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
			}
		}
		if (codes.empty())
		{
			std::stringstream ss;
			ss << "No status page codes: " << directive.value << " line: " << directive.line
			   << " column: " << directive.column << " skipping..." << std::endl;
			Logger::warning(ss.str());
			return;
		}
		else if ((*it)->type == AST::NodeType::ARG)
		{
			if ((*it)->value.empty())
			{
				std::stringstream ss;
				ss << "Empty status page path: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			if ((*it)->value[0] != '/')
			{
				std::stringstream ss;
				ss << "Status page path must start with slash: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			if ((*it)->value.find("..") != std::string::npos)
			{
				std::stringstream ss;
				ss << "Status page path contains traversal: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			if ((*it)->value.find(" ") != std::string::npos)
			{
				std::stringstream ss;
				ss << "Status page path contains spaces: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << " skipping..." << std::endl;
				Logger::warning(ss.str());
				return;
			}
			server.insertStatusPage((*it)->value, codes);
		}
	}
	catch (const std::exception &e)
	{
		std::stringstream ss;
		ss << "Error translating status page directive: " << e.what() << " line: " << directive.line
		   << " column: " << directive.column << " skipping..." << std::endl;
		Logger::warning(ss.str());
		return;
	}
}
/*
** --------------------------------- LOCATION SPECIFIC HELPERS ---------------------------------
*/

// Location verification
void ConfigTranslator::_translateLocation(const AST::ASTNode &location_node)
{
	Location location;
	for (std::vector<AST::ASTNode *>::const_iterator it = location_node.children.begin();
		 it != location_node.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::DIRECTIVE)
		{
			if ((*it)->value == "root")
				_translateLocationRoot(**it, location_node.value);
			else if ((*it)->value == "allowed_methods")
				_translateLocationAllowedMethods(**it, location_node.value);
			else if ((*it)->value == "return")
				_translateLocationReturn(**it, location_node.value);
			else if ((*it)->value == "autoindex")
				_translateLocationAutoindex(**it, location_node.value);
			else if ((*it)->value == "index")
				_translateLocationIndex(**it, location_node.value);
			else if ((*it)->value == "cgi_path")
				_translateLocationCgiPath(**it, location_node.value);
			else if ((*it)->value == "cgi_param")
				_translateLocationCgiParam(**it, location_node.value);
			else if ((*it)->value == "cgi_index")
				_translateLocationCgiIndex(**it, location_node.value);
			else
			{
				std::stringstream ss;
				ss << "Unknown directive in location block: " << (*it)->value << " line: " << (*it)->line
				   << " column: " << (*it)->column << std::endl;
				Logger::warning(ss.str());
			}
		}
	}
	else
	{
		std::stringstream ss;
		ss << "Unknown token in location block: " << (*it)->value << " line: " << (*it)->line
		   << " column: " << (*it)->column << std::endl;
		Logger::warning(ss.str());
	}
}