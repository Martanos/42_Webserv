#include "../../includes/ConfigParser/ConfigParser.hpp"
#include <sstream>
#include <stdexcept>

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

ConfigParser::ConfigParser(ConfigTokeniser &tok) : _tok(&tok)
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

ConfigParser::~ConfigParser()
{
}

/*
** --------------------------------- Private Helpers----------------------------------
*/

Token::Token ConfigParser::expect(Token::TokenType type, const char *what)
{
	Token::Token t = _tok->nextToken();
	if (t.type != type)
	{
		std::ostringstream ss;
		if (what)
			ss << "Expected " << what << " but found '" << t.lexeme << "' at " << t.line << ":" << t.column;
		else
			ss << "Unexpected token '" << t.lexeme << "' at " << t.line << ":" << t.column;
		throw std::runtime_error(ss.str());
	}
	return t;
}

bool ConfigParser::accept(Token::TokenType type, Token::Token *out)
{
	Token::Token t = _tok->peek(1);
	if (t.type == type)
	{
		_tok->nextToken();
		if (out)
			*out = t;
		return true;
	}
	return false;
}

void ConfigParser::parseServerBlock(AST::Config &cfg)
{
	Token::Token open = expect(Token::TOKEN_OPEN_BRACE, "'{' after server");
	AST::Server srv;
	srv.line = open.line;
	srv.column = open.column;

	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_CLOSE_BRACE)
		{
			_tok->nextToken(); // consume '}'
			break;
		}
		if (t.type == Token::TOKEN_EOF)
		{
			std::ostringstream ss;
			ss << "Unterminated server block starting at " << srv.line << ":" << srv.column;
			throw std::runtime_error(ss.str());
		}
		if (t.type == Token::TOKEN_IDENTIFIER && t.lexeme == "location")
		{
			_tok->nextToken(); // consume 'location'
			AST::Location loc = parseLocation();
			srv.locations.push_back(loc);
			continue;
		}
		AST::Directive d = parseDirective();
		srv.directives.push_back(d);
	}

	cfg.servers.push_back(srv);
}

AST::Directive ConfigParser::parseDirective()
{
	Token::Token name = expect(Token::TOKEN_IDENTIFIER, "directive name");
	AST::Directive d;
	d.name = name.lexeme;
	d.line = name.line;
	d.column = name.column;

	// gather args until ';' or block '{'
	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_SEMICOLON)
		{
			_tok->nextToken(); // consume ';'
			return d;
		}
		if (t.type == Token::TOKEN_OPEN_BRACE)
		{
			_tok->nextToken(); // consume '{'
			// parse inner directives until }
			parseDirectivesInto(d.args.empty() ? /*temporary vector */ *new std::vector<AST::Directive>()
											   : *new std::vector<AST::Directive>());
			// Note: in this simplified sketch we don't attach nested directives to Directive.
			// Typical nginx uses block directives (like location) handled explicitly; keep simple here.
			// Consume matching '}' handled by parseDirectivesInto.
			return d;
		}
		if (t.type == Token::TOKEN_IDENTIFIER || t.type == Token::TOKEN_NUMBER || t.type == Token::TOKEN_STRING)
		{
			Token::Token arg = _tok->nextToken();
			d.args.push_back(arg.lexeme);
			continue;
		}
		std::ostringstream ss;
		ss << "Unexpected token in directive '" << d.name << "': '" << t.lexeme << "' at " << t.line << ":" << t.column;
		throw std::runtime_error(ss.str());
	}
}

// parse directives into a vector until matching closing brace
void ConfigParser::parseDirectivesInto(std::vector<AST::Directive> &out)
{
	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_CLOSE_BRACE)
		{
			_tok->nextToken(); // consume '}'
			return;
		}
		if (t.type == Token::TOKEN_EOF)
		{
			throw std::runtime_error("Unterminated block (expected '}' before EOF)");
		}
		if (t.type == Token::TOKEN_IDENTIFIER && t.lexeme == "location")
		{
			_tok->nextToken(); // consume 'location'
			AST::Location loc = parseLocation();
			// Convert location into a Directive-like wrapper if desired, here we ignore
			AST::Directive d;
			d.name = "location";
			d.args.push_back(loc.path);
			d.line = loc.line;
			d.column = loc.column;
			out.push_back(d);
			continue;
		}
		AST::Directive d = parseDirective();
		out.push_back(d);
	}
}

AST::Location ConfigParser::parseLocation()
{
	Token::Token pathTok = expect(Token::TOKEN_IDENTIFIER, "location path");
	AST::Location loc;
	loc.path = pathTok.lexeme;
	loc.line = pathTok.line;
	loc.column = pathTok.column;

	Token::Token open = expect(Token::TOKEN_OPEN_BRACE, "'{' after location");
	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_CLOSE_BRACE)
		{
			_tok->nextToken(); // consume '}'
			break;
		}
		if (t.type == Token::TOKEN_EOF)
		{
			std::ostringstream ss;
			ss << "Unterminated location block at " << loc.line << ":" << loc.column;
			throw std::runtime_error(ss.str());
		}
		AST::Directive d = parseDirective();
		loc.directives.push_back(d);
	}
	return loc;
}

/*
** --------------------------------- METHODS ----------------------------------
*/
AST::Config ConfigParser::parse()
{
	AST::Config cfg;
	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_EOF)
		{
			_tok->nextToken();
			break;
		}
		if (t.type == Token::TOKEN_IDENTIFIER && t.lexeme == "server")
		{
			_tok->nextToken(); // consume 'server'
			parseServerBlock(cfg);
			continue;
		}
		std::ostringstream ss;
		ss << "Top-level: unexpected token '" << t.lexeme << "' at " << t.line << ":" << t.column;
		throw std::runtime_error(ss.str());
	}
	return cfg;
}

void ConfigParser::printAST(const AST::Config &cfg) const
{
	std::cout << "AST:" << std::endl;
	for (size_t i = 0; i < cfg.servers.size(); ++i)
	{
		const AST::Server &srv = cfg.servers[i];
		std::cout << "Server " << i << " (line " << srv.line << ", column " << srv.column << "):" << std::endl;

		// Print top-level directives
		for (size_t d = 0; d < srv.directives.size(); ++d)
		{
			const AST::Directive &dir = srv.directives[d];
			std::cout << "  Directive " << d << ": " << dir.name << " (line " << dir.line << ", column " << dir.column
					  << ")";
			if (!dir.args.empty())
			{
				std::cout << " [args:";
				for (size_t a = 0; a < dir.args.size(); ++a)
					std::cout << " " << dir.args[a];
				std::cout << "]";
			}
			std::cout << std::endl;
		}

		// Print locations
		for (size_t j = 0; j < srv.locations.size(); ++j)
		{
			const AST::Location &loc = srv.locations[j];
			std::cout << "  Location " << j << ": \"" << loc.path << "\" (line " << loc.line << ", column "
					  << loc.column << ")" << std::endl;

			for (size_t k = 0; k < loc.directives.size(); ++k)
			{
				const AST::Directive &dir = loc.directives[k];
				std::cout << "    Directive " << k << ": " << dir.name << " (line " << dir.line << ", column "
						  << dir.column << ")";
				if (!dir.args.empty())
				{
					std::cout << " [args:";
					for (size_t a = 0; a < dir.args.size(); ++a)
						std::cout << " {" << dir.args[a] << "}";
					std::cout << "]";
				}
				std::cout << std::endl;
			}
		}
	}
	std::cout << std::endl;
}

/* ************************************************************************** */