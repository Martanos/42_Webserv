#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include "ConfigNameSpace.hpp"
#include "ConfigTokeniser.hpp"
#include <iostream>
#include <string>
#include <vector>

// Builds the AST tree
class ConfigParser
{
private:
	ConfigTokeniser *_tok; // non-owning borrowed pointer

	// helpers
	Token::Token expect(Token::TokenType type, const char *what = 0);
	bool accept(Token::TokenType type, Token::Token *out = 0);

	void parseServerBlock(AST::ASTNode &cfg);
	AST::ASTNode *parseDirective();
	AST::ASTNode *parseLocation();

	// Non-copyable
	ConfigParser(ConfigParser const &);
	ConfigParser &operator=(ConfigParser const &);

public:
	explicit ConfigParser(ConfigTokeniser &tok);
	~ConfigParser();

	// Parse entire config; throws std::runtime_error on syntax error.
	AST::ASTNode parse();

	// Diagnosis Method
	void printAST(const AST::ASTNode &cfg) const;
	void printASTRecursive(const AST::ASTNode &node, int depth) const;
};

#endif /* **************************************************** CONFIGPARSER_H */