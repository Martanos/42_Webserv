#ifndef CONFIG_NAMESPACE_HPP
#define CONFIG_NAMESPACE_HPP

#include <string>

namespace Token
{
    enum TokenType
    {
        TOKEN_KEYWORD,     // e.g. "server", "listen"
        TOKEN_VALUE,       // e.g. "8080", "/var/www"
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

#endif
