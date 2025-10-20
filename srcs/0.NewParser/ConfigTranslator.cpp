#include "../../includes/ConfigParser/ConfigTranslator.hpp"
#include <vector>

ConfigTranslator::ConfigTranslator(const AST::ASTNode &ast)
{
	translate(ast);
}

ConfigTranslator::~ConfigTranslator()
{
}

const std::vector<Server> &ConfigTranslator::getServers() const
{
	return _servers;
}

void ConfigTranslator::translate(const AST::ASTNode &ast)
{
	// Translate Server Directives first
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::SERVER)
		{
			Server server = translateServer(**it);
		}
	}
}

void ConfigTranslator::translateServer(const AST::ASTNode &ast)
{
	Server server;
	// Translate Server Directives
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::DIRECTIVE)
		{
			if ((*it)->value == "listen")
			{
			}
			else if ((*it)->value == "server_name")
			{
			}
			else if ((*it)->value == "root")
			{
			}
			else if ((*it)->value == "index")
			{
			}
		}
	}
	// Translate Server Locations
	for (std::vector<AST::ASTNode *>::const_iterator it = ast.children.begin(); it != ast.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::LOCATION)
		{
			translateLocation(*it, server);
		}
	}
	return server;
}

void translateServerName(const AST::ASTNode &directive, Server &server)
{
	for (std::vector<AST::ASTNode *> const_iterator it = directive.children.begin(); it != directive.children.end();
		 ++it)
	{
		if (server.getServerName((*it).value) == NULL)
		{
			server.insertServerName((*it).value);
		}
		else
		{
			stringstream ss;
			ss << "Duplicate server name detected : " << (*it).value << " line: " << (*it).line
			   << " column: " << (*it).column << std::endl;
			Logger::warning(ss.str());
		}
	}
}

void translateHostPort(const AST::ASTNode &directive, Server &server)
{
}