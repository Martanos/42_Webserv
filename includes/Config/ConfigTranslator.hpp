#ifndef CONFIG_TRANSLATOR_HPP
#define CONFIG_TRANSLATOR_HPP

#include "../../includes/Config/ConfigNameSpace.hpp"
#include "../../includes/Config/Location.hpp"
#include "../../includes/Config/Server.hpp"
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

	// Server specific translation helpers
	Server _translateServer(const AST::ASTNode &ast);
	void _translateServerName(const AST::ASTNode &directive, Server &server);
	void _translateListen(const AST::ASTNode &directive, Server &server);

	// Location specific translation helpers
	void _translateLocation(const AST::ASTNode &location_node, Location &location);

	// Directive translation helpers
	void _translateDirective(std::vector<AST::ASTNode *>::const_iterator &directive, Directives &directives,
							 std::string context);
	void _translateRootPathDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateAutoindexDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateCgiPathDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateClientMaxBodySizeDirective(const AST::ASTNode &directive, Directives &directives,
											  std::string context);
	void _translateKeepAliveDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateRedirectDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateIndexDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateStatusPathDirective(const AST::ASTNode &directive, Directives &directives, std::string context);
	void _translateAllowedMethodsDirective(const AST::ASTNode &directive, Directives &directives, std::string context);

	// Utility
	bool _parseSizeArgument(const std::string &rawValue, double &sizeOut);

public:
	explicit ConfigTranslator(const AST::ASTNode &ast);
	~ConfigTranslator();

	// Accessors
	const std::vector<Server> &getServers() const;
};

#endif /* **************************************************** CONFIG_TRANSLATOR_H */
