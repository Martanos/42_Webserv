#ifndef TOKENISER_HPP
#define TOKENISER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "ConfigFileReader.hpp"

/// @brief Tokeniser for the config file
class ConfigTokeniser
{
	// Publicly accessible enum for token types
public:
	enum TokenType
	{
		TOKEN_KEYWORD,	   // e.g. "server", "listen"
		TOKEN_VALUE,	   // e.g. "8080", "/var/www"
		TOKEN_OPEN_BRACE,  // {
		TOKEN_CLOSE_BRACE, // }
		TOKEN_SEMICOLON,   // ;
		TOKEN_END_OF_FILE
	};

	struct Token
	{
		TokenType type;
		std::string text;
		int line;
		int column;

		Token(TokenType type, const std::string &text, int line, int column)
			: type(type), text(text), line(line), column(column)
		{
		}
	};

private:
	ConfigFileReader &reader;
	std::string currentLine;
	size_t pos;
	int lineNumber;

	// Non instantiable without a reader
	ConfigTokeniser();

	// Non copyable
	ConfigTokeniser(ConfigTokeniser const &src);
	ConfigTokeniser &operator=(ConfigTokeniser const &rhs);

	// Tokeniser methods
	Token consumeSingle(TokenType type)
	{
		char c = currentLine[pos++];
		return Token(type, std::string(1, c), lineNumber, pos);
	}

public:
	ConfigTokeniser(ConfigFileReader &reader);
	~ConfigTokeniser();

	Token nextToken()
	{
		while (true)
		{
			// refill line if needed
			if (pos >= currentLine.size())
			{
				if (!reader.nextLine(currentLine))
					return Token(TOKEN_END_OF_FILE, "", lineNumber, pos);
				lineNumber = reader.getLineNumber();
				pos = 0;
				continue;
			}

			char c = currentLine[pos];

			// skip whitespace
			if (std::isspace(static_cast<unsigned char>(c)))
			{
				++pos;
				continue;
			}

			// skip comments (# until end of line)
			if (c == '#')
			{
				currentLine.clear();
				pos = 0;
				continue;
			}

			// single-character tokens
			if (c == '{')
				return consumeSingle(TOKEN_OPEN_BRACE);
			if (c == '}')
				return consumeSingle(TOKEN_CLOSE_BRACE);
			if (c == ';')
				return consumeSingle(TOKEN_SEMICOLON);

			// word (keyword or value)
			if (std::isgraph(static_cast<unsigned char>(c)))
			{
				int start = pos;
				while (pos < currentLine.size() &&
					   !std::isspace(static_cast<unsigned char>(currentLine[pos])) &&
					   currentLine[pos] != '{' &&
					   currentLine[pos] != '}' &&
					   currentLine[pos] != ';')
				{
					++pos;
				}
				std::string word = currentLine.substr(start, pos - start);
				return Token(TOKEN_KEYWORD, word, lineNumber, start + 1);
			}

			throw std::runtime_error("Unexpected character in config");
		}
	}
};

#endif /* ******************************************************* TOKENISER_H */