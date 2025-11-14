#ifndef CONFIG_TOKENISER_HPP
#define CONFIG_TOKENISER_HPP

#include "ConfigFileReader.hpp"
#include "ConfigNameSpace.hpp"
#include <deque>
#include <string>

// Tokeniser for the config file
class ConfigTokeniser
{
private:
	ConfigFileReader *_reader; // non-owning borrowed pointer
	std::string _currentLine;  // current line contents
	size_t _pos;			   // 0-based index into _currentLine
	size_t _lineNumber;		   // 1-based line number for currentLine
	size_t _columnBase;		   // usually 1; start column base for currentLine
	std::deque<Token::Token> _lookahead;

	// internal helpers
	bool refillLine();								   // reads next line into _currentLine, returns false on EOF
	Token::Token consumeSingle(Token::TokenType type); // consume single-char tokens { } ;
	Token::Token lexWord();							   // identifier or number
	Token::Token lexString();						   // parses double-quoted string with escapes
	static bool isIdentChar(unsigned char ch);
	static bool isDigit(unsigned char ch);

	// Non-copyable
	ConfigTokeniser(ConfigTokeniser const &);
	ConfigTokeniser &operator=(ConfigTokeniser const &);

public:
	explicit ConfigTokeniser(ConfigFileReader &reader);
	~ConfigTokeniser();

	// Consume and return next token
	Token::Token nextToken();

	// Lookahead without consuming. n==1 returns next token.
	Token::Token peek(size_t n = 1);

	// Push a token back onto the front of the stream (for simple parser backtracking)
	void pushback(const Token::Token &t);
};

#endif /* ******************************************************* CONFIG_TOKENISER_H */