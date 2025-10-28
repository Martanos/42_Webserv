#ifndef CONFIG_NAMESPACE_HPP
#define CONFIG_NAMESPACE_HPP

#include <cstddef>
#include <cstring>
#include <limits.h>
#include <netinet/in.h>
#include <string>
#include <vector>

// Shared namespace for parser classes
namespace Token
{
enum TokenType
{
	TOKEN_EOF,
	TOKEN_OPEN_BRACE,  // {
	TOKEN_CLOSE_BRACE, // }
	TOKEN_SEMICOLON,   // ;
	TOKEN_IDENTIFIER,  // keywords, names, paths
	TOKEN_NUMBER,	   // numeric literal (digits only)
	TOKEN_STRING,	   // double-quoted with escapes interpreted
	TOKEN_ERROR		   // lexical error with message in message
};

struct Token
{
	TokenType type;
	std::string lexeme;	 // interpreted text for strings, raw text for others
	size_t line;		 // 1-based
	size_t column;		 // 1-based start column (byte offset)
	std::string message; // only used for TOKEN_ERROR

	Token() : type(TOKEN_EOF), line(0), column(0)
	{
	}
	Token(TokenType t, const std::string &s, size_t l, size_t c) : type(t), lexeme(s), line(l), column(c)
	{
	}
	static Token Error(const std::string &msg, size_t l, size_t c)
	{
		Token t;
		t.type = TOKEN_ERROR;
		t.message = msg;
		t.line = l;
		t.column = c;
		return t;
	}
};
} // namespace Token

namespace AST
{

enum NodeType
{
	UNKNOWN = 0,
	CONFIG = 1,
	SERVER = 2,
	DIRECTIVE = 3,
	LOCATION = 4,
	ARG = 5,
	ERROR = 6
};

struct ASTNode
{
	NodeType type;
	std::string value;
	size_t line;
	size_t column;
	size_t position;
	std::string message; // For errors;
	std::vector<ASTNode *> children;

	// Constructor
	ASTNode(NodeType t, const std::string &v = "") : type(t), value(v)
	{
	}

	// Destructor cleans up children
	~ASTNode()
	{
		for (size_t i = 0; i < children.size(); ++i)
		{
			delete children[i];
		}
	}

	// Utility
	void addChild(ASTNode *child)
	{
		children.push_back(child);
	}

	void Error(std::string message)
	{
		this->type = ERROR;
		this->message = message;
	}
};

} // namespace AST

#endif
