#ifndef CONFIG_TRANSLATOR_HPP
#define CONFIG_TRANSLATOR_HPP

#include "../../includes/Core/Server.hpp"
#include "ConfigNameSpace.hpp"
#include <vector>

class ConfigTranslator
{
private:
	std::vector<Server> _servers; // owned

	// Non-copyable
	ConfigTranslator(const ConfigTranslator &other);
	ConfigTranslator &operator=(const ConfigTranslator &other);

	// Server object creation
	void _translate(const AST::ASTNode &ast);
	void _translateServer(const AST::ASTNode &ast);
	void _translateServerNames(const AST::ASTNode &directive, Server &server);
	void _translateHostPorts(const AST::ASTNode &directive, Server &server);
	void _translateIndex(const AST::ASTNode &directive, Server &server);
	void _translateAutoIndex(const AST::ASTNode &directive, Server &server);
	void _translatePages(const AST::ASTNode &directive, Server &server);
	void _translateKeepAlive(const AST::ASTNode &directive, Server &server);
	void _translateClientMaxUriSize(const AST::ASTNode &directive, Server &server);
	void _translateClientMaxHeadersSize(const AST::ASTNode &directive, Server &server);
	void _translateClientMaxBodySize(const AST::ASTNode &directive, Server &server);
	void _translateRoot(const AST::ASTNode &directive, Server &server);

	// Location helpers
	void _translateLocation(const AST::ASTNode &directive, Server &server);

public:
	explicit ConfigTranslator(const AST::ASTNode &ast);
	~ConfigTranslator();

	const std::vector<Server> &getServers() const;
};

#endif /* **************************************************** CONFIG_TRANSLATOR_H */