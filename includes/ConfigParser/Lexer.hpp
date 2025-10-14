#ifndef LEXER_HPP
#define LEXER_HPP

#include <string>
#include <vector>

class Lexer
{
public:
	enum TokenType
	{
		TOKEN_KEYWORD,
		TOKEN_VALUE,
		TOKEN_OPEN_BRACE,
		TOKEN_CLOSE_BRACE,
		TOKEN_SEMICOLON,
		TOKEN_END_OF_FILE
	};

	struct Token
	{
		TokenType type;
		std::string text;
		int lineNumber;
		int columnNumber;
	};

private:
	std::string input_;
	size_t pos_;
	int line_;
	int column_;

	char peek() const;
	char advance();
	void skipWhitespace();
	void skipComment();
	Token readWord();

public:
	Lexer();
	Lexer(const std::string &input);
	~Lexer();

	std::vector<Token> tokenize();
};

#endif