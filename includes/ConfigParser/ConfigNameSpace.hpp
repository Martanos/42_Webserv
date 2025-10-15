#ifndef CONFIG_NAMESPACE_HPP
#define CONFIG_NAMESPACE_HPP

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
struct Directive
{
	std::string name;
	std::vector<std::string> args;
	size_t line;
	size_t column;
};

struct Location
{
	std::string path;
	std::vector<Directive> directives;
	size_t line;
	size_t column;
};

struct Server
{
	std::vector<Directive> directives;
	std::vector<Location> locations;
	size_t line;
	size_t column;
};

struct Config
{
	std::vector<Server> servers;
};
} // namespace AST

#endif
