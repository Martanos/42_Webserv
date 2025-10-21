#ifndef CONFIG_TRANSLATOR_HPP
#define CONFIG_TRANSLATOR_HPP

#include "../../includes/ConfigParser/ConfigNameSpace.hpp"
#include "../../includes/Core/Location.hpp"
#include "../../includes/Core/Server.hpp"
#include <vector>

class ConfigTranslator
{
private:
	std::vector<Server> _servers; // owned

	// Non-copyable
	ConfigTranslator(const ConfigTranslator &other);
	ConfigTranslator &operator=(const ConfigTranslator &other);

	// Translation helpers
	void _translate(const AST::ASTNode &ast);
	Server _translateServer(const AST::ASTNode &ast);

	// Server specific translation helpers
	void _translateServerName(const AST::ASTNode &directive, Server &server);
	void _translateListen(const AST::ASTNode &directive, Server &server);
	void _translateServerRoot(const AST::ASTNode &directive, Server &server);
	void _translateServerIndex(const AST::ASTNode &directive, Server &server);
	void _translateServerAutoindex(const AST::ASTNode &directive, Server &server);
	void _translateServerClientMaxBodySize(const AST::ASTNode &directive, Server &server);
	void _translateServerClientMaxHeadersSize(const AST::ASTNode &directive, Server &server);
	void _translateServerClientMaxUriSize(const AST::ASTNode &directive, Server &server);
	void _translateServerStatusPage(const AST::ASTNode &directive, Server &server);

	// Location specific translation helpers
	Location _translateLocation(const AST::ASTNode &location_node);
	void _translateLocationRoot(const AST::ASTNode &directive, Location &location);
	void _translateLocationAllowedMethods(const AST::ASTNode &directive, Location &location);
	void _translateLocationReturn(const AST::ASTNode &directive, Location &location);
	void _translateLocationAutoindex(const AST::ASTNode &directive, Location &location);
	void _translateLocationIndex(const AST::ASTNode &directive, Location &location);
	void _translateLocationCgiPath(const AST::ASTNode &directive, Location &location);

public:
	explicit ConfigTranslator(const AST::ASTNode &ast);
	~ConfigTranslator();

	const std::vector<Server> &getServers() const;
};

#endif /* **************************************************** CONFIG_TRANSLATOR_H */