#include "../../includes/Config/ConfigTokeniser.hpp"
/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigTokeniser::ConfigTokeniser(ConfigFileReader &reader)
	: _reader(&reader), _currentLine(), _pos(0), _lineNumber(0), _columnBase(1), _lookahead()
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigTokeniser::~ConfigTokeniser()
{
}

/*
** --------------------------------- Internal Methods --------------------------------
*/

bool ConfigTokeniser::refillLine()
{
	_currentLine.clear();
	_pos = 0;
	if (!_reader)
		return false;
	if (!_reader->nextLine(_currentLine))
		return false;
	_lineNumber = _reader->getLineNumber();
	_columnBase = 1;
	return true;
}

Token::Token ConfigTokeniser::consumeSingle(Token::TokenType type)
{
	size_t col = _pos + _columnBase;
	char c = _currentLine[_pos++];
	std::string s(1, c);
	return Token::Token(type, s, _lineNumber, col);
}

bool ConfigTokeniser::isIdentChar(unsigned char ch)
{
	return std::isalnum(ch) || ch == '_' || ch == '-' || ch == '.' || ch == '/' || ch == '$' || ch == ':' ||
		   ch == '[' || ch == ']';
}

bool ConfigTokeniser::isDigit(unsigned char ch)
{
	return ch >= '0' && ch <= '9';
}

// Lex a word (identifier or number)
Token::Token ConfigTokeniser::lexWord()
{
	size_t start = _pos;
	size_t col = start + _columnBase;
	while (_pos < _currentLine.size())
	{
		unsigned char uch = static_cast<unsigned char>(_currentLine[_pos]);
		if (!isIdentChar(uch))
			break;
		++_pos;
	}
	std::string word = _currentLine.substr(start, _pos - start);
	// classify number vs identifier
	for (size_t i = 0; i < word.size(); ++i)
	{
		if (!isDigit(static_cast<unsigned char>(word[i])))
			return Token::Token(Token::TOKEN_IDENTIFIER, word, _lineNumber, col);
	}
	return Token::Token(Token::TOKEN_NUMBER, word, _lineNumber, col);
}

// Lex a double-quoted string with escapes interpreted
Token::Token ConfigTokeniser::lexString()
{
	// current char is '"'
	size_t startCol = _pos + _columnBase;
	++_pos; // consume opening quote
	std::string out;
	while (true)
	{
		if (_pos >= _currentLine.size())
		{
			return Token::Token::Error("Unterminated string literal", _lineNumber, startCol);
		}
		char ch = _currentLine[_pos++];
		if (ch == '"')
		{
			return Token::Token(Token::TOKEN_STRING, out, _lineNumber, startCol);
		}
		if (ch == '\\')
		{
			if (_pos >= _currentLine.size())
			{
				return Token::Token::Error("Invalid escape at end of line", _lineNumber, _pos + _columnBase);
			}
			char esc = _currentLine[_pos++];
			switch (esc)
			{
			case 'n':
				out.push_back('\n');
				break;
			case 't':
				out.push_back('\t');
				break;
			case 'r':
				out.push_back('\r');
				break;
			case '\\':
				out.push_back('\\');
				break;
			case '"':
				out.push_back('"');
				break;
			default:
				// unknown escape: keep escaped char
				out.push_back(esc);
				break;
			}
			continue;
		}
		// normal character
		out.push_back(ch);
	}
}

/*
** --------------------------------- METHODS ----------------------------------
*/

Token::Token ConfigTokeniser::nextToken()
{
	if (!_lookahead.empty())
	{
		Token::Token t = _lookahead.front();
		_lookahead.pop_front();
		return t;
	}

	while (true)
	{
		// Refill until we have either EOF or a non-empty line buffer to inspect
		while (_pos >= _currentLine.size())
		{
			if (!refillLine())
			{
				size_t reportLine = _lineNumber ? _lineNumber : 1;
				return Token::Token(Token::TOKEN_EOF, std::string(), reportLine, 1);
			}
			// freshly refilled line: reset pos/column if refillLine didn't already
			_pos = 0;
			_columnBase = 1;
		}

		// If the current line contains only whitespace, skip it entirely
		bool allws = true;
		for (size_t i = 0; i < _currentLine.size(); ++i)
		{
			if (!std::isspace(static_cast<unsigned char>(_currentLine[i])))
			{
				allws = false;
				break;
			}
		}
		if (allws)
		{
			_currentLine.clear();
			_pos = 0;
			continue;
		}

		// Safe to index as _pos < _currentLine.size() and line not all-whitespace
		unsigned char uch = static_cast<unsigned char>(_currentLine[_pos]);

		// whitespace within line: consume and continue
		if (std::isspace(uch))
		{
			++_pos;
			continue;
		}

		// comments: skip rest of line on '#'
		if (_currentLine[_pos] == '#')
		{
			_currentLine.clear();
			_pos = 0;
			continue;
		}

		char c = _currentLine[_pos];

		// punctuation
		if (c == '{')
			return consumeSingle(Token::TOKEN_OPEN_BRACE);
		if (c == '}')
			return consumeSingle(Token::TOKEN_CLOSE_BRACE);
		if (c == ';')
			return consumeSingle(Token::TOKEN_SEMICOLON);

		// string literal
		if (c == '"')
			return lexString();

		// identifier / number / path
		if (isIdentChar(uch))
			return lexWord();

		// Unexpected single character -> emit an error token containing that character
		size_t col = _pos + _columnBase;
		std::string msg = std::string("Unexpected character: ") + c;
		++_pos;
		return Token::Token::Error(msg, _lineNumber, col);
	}
}
Token::Token ConfigTokeniser::peek(size_t n)
{
	while (_lookahead.size() < n)
	{
		Token::Token t = nextToken();
		_lookahead.push_back(t);
		if (t.type == Token::TOKEN_EOF || t.type == Token::TOKEN_ERROR)
			break;
	}
	if (_lookahead.size() >= n)
		return _lookahead[n - 1];
	// fallback (shouldn't happen)
	return Token::Token(Token::TOKEN_EOF, std::string(), _lineNumber ? _lineNumber : 1, 1);
}

void ConfigTokeniser::pushback(const Token::Token &t)
{
	_lookahead.push_front(t);
}
