#ifndef TOKENISER_HPP
#define TOKENISER_HPP

#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "ConfigFileReader.hpp"
#include "ConfigNameSpace.hpp"
/// @brief Tokeniser for the config file
class ConfigTokeniser
{
private:
	ConfigFileReader reader;
	std::string currentLine;
	size_t pos;
	int lineNumber;

	// Non instantiable without a reader
	ConfigTokeniser();

	// Non copyable
	ConfigTokeniser(ConfigTokeniser const &src);
	ConfigTokeniser &operator=(ConfigTokeniser const &rhs);

	// Tokeniser methods
	Token::Token consumeSingle(Token::TokenType type)
	{
		char c = currentLine[pos++];
		return Token::Token(type, std::string(1, c), lineNumber, pos);
	}

public:
	ConfigTokeniser(ConfigFileReader &reader);
	~ConfigTokeniser();

	Token::Token nextToken()
	{
		while (true)
		{
			// refill line if needed
			if (pos >= currentLine.size())
			{
				if (!reader.nextLine(currentLine))
					return Token::Token(Token::TOKEN_END_OF_FILE, "", lineNumber, pos);
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
				return consumeSingle(Token::TOKEN_OPEN_BRACE);
			if (c == '}')
				return consumeSingle(Token::TOKEN_CLOSE_BRACE);
			if (c == ';')
				return consumeSingle(Token::TOKEN_SEMICOLON);

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
				return Token::Token(Token::TOKEN_KEYWORD, word, lineNumber, start + 1);
			}

			throw std::runtime_error("Unexpected character in config");
		}
	}
};

#endif /* ******************************************************* TOKENISER_HPP */