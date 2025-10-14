#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <iostream>
#include <string>
#include <vector>
#include "ConfigTokeniser.hpp"

// Builds the AST tree
class ConfigParser
{
	// Publicly accessible nodes
public:
	struct DirectiveNode
	{
		std::string name;
		std::vector<std::string> args;
		int line;

		// Canonical form
		DirectiveNode() : name(""), args(), line(0) {}
		DirectiveNode(const std::string &n, const std::vector<std::string> &a, int l)
			: name(n), args(a), line(l) {}
		DirectiveNode(const DirectiveNode &other)
			: name(other.name), args(other.args), line(other.line) {}
		DirectiveNode &operator=(const DirectiveNode &rhs)
		{
			if (this != &rhs)
			{
				name = rhs.name;
				args = rhs.args;
				line = rhs.line;
			}
			return *this;
		}
		~DirectiveNode() {}
	};

	struct BlockNode
	{
		std::string type;
		std::string selector;
		std::vector<DirectiveNode> directives;
		std::vector<BlockNode> children;
		int line;

		// Canonical form
		BlockNode() : type(""), selector(""), directives(), children(), line(0) {}
		BlockNode(const std::string &t, int l)
			: type(t), selector(""), directives(), children(), line(l) {}
		BlockNode(const BlockNode &other)
			: type(other.type), selector(other.selector),
			  directives(other.directives), children(other.children),
			  line(other.line) {}
		BlockNode &operator=(const BlockNode &rhs)
		{
			if (this != &rhs)
			{
				type = rhs.type;
				selector = rhs.selector;
				directives = rhs.directives;
				children = rhs.children;
				line = rhs.line;
			}
			return *this;
		}
		~BlockNode() {}
	};

public:
	// Canonical form
	ConfigParser(ConfigTokeniser &tokenizer)
		: tokenizer(tokenizer), lookaheadValid(false) {}
	ConfigParser(const ConfigParser &other)
		: tokenizer(other.tokenizer), lookahead(other.lookahead),
		  lookaheadValid(other.lookaheadValid) {}
	ConfigParser &operator=(const ConfigParser &rhs)
	{
		if (this != &rhs)
		{
			tokenizer = rhs.tokenizer;
			lookahead = rhs.lookahead;
			lookaheadValid = rhs.lookaheadValid;
		}
		return *this;
	}
	~ConfigParser() {}

	std::vector<BlockNode> parse();

private:
	ConfigTokeniser &tokenizer; // reference, so assignment is tricky
	ConfigTokeniser::Token lookahead;
	bool lookaheadValid;

	ConfigTokeniser::Token next();
	ConfigTokeniser::Token peek();
	BlockNode parseBlock();
	DirectiveNode parseDirective();
};

std::ostream &operator<<(std::ostream &o, ConfigParser const &i);

#endif /* **************************************************** CONFIGPARSER_H */