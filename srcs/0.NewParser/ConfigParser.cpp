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

void ConfigParser::parseServerBlock(AST::ASTNode &cfg)
{
	Token::Token open = expect(Token::TOKEN_OPEN_BRACE, "'{' after server");
	AST::ASTNode srv(AST::NodeType::SERVER);
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
			AST::ASTNode loc = parseLocation();
			srv.addChild(&loc);
			continue;
		}
		AST::ASTNode d = parseDirective();
		srv.addChild(&d);
	}

	cfg.addChild(&srv);
}

AST::ASTNode ConfigParser::parseDirective()
{
	Token::Token name = expect(Token::TOKEN_IDENTIFIER, "directive name");
	AST::ASTNode d(AST::NodeType::DIRECTIVE);
	d.value = name.lexeme;
	d.line = name.line;
	d.column = name.column;

	// gather args until ';'
	while (true)
	{
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_SEMICOLON)
		{
			_tok->nextToken(); // consume ';'
			return d;
		}
		else if (t.type == Token::TOKEN_IDENTIFIER || t.type == Token::TOKEN_NUMBER || t.type == Token::TOKEN_STRING)
		{
			Token::Token arg = _tok->nextToken();
			AST::ASTNode directiveArg(AST::NodeType::ARG, arg.lexeme);
			d.addChild(&directiveArg);
			continue;
		}
		std::ostringstream ss;
		ss << "Unexpected token in directive '" << d.value << "': '" << t.lexeme << "' at " << t.line << ":"
		   << t.column;
		throw std::runtime_error(ss.str());
	}
}

AST::ASTNode ConfigParser::parseLocation()
{
	Token::Token pathTok = expect(Token::TOKEN_IDENTIFIER, "location path");
	AST::ASTNode loc(AST::NodeType::LOCATION);
	loc.value = pathTok.lexeme;
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
		AST::ASTNode d = parseDirective();
		loc.addChild(&d);
	}
	return loc;
}

/*
** --------------------------------- METHODS ----------------------------------
*/
AST::ASTNode ConfigParser::parse()
{
	AST::ASTNode cfg(AST::NodeType::CONFIG);
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

// Recursive descent print
void ConfigParser::printAST(const AST::ASTNode &cfg) const
{
	printASTRecursive(cfg, 0);
}

void ConfigParser::printASTRecursive(const AST::ASTNode &node, int depth) const
{
	// Print indentation based on depth
	for (int i = 0; i < depth; ++i)
		std::cout << "  ";

	// Print node type and value
	switch (node.type)
	{
	case AST::NodeType::CONFIG:
		std::cout << "CONFIG";
		break;
	case AST::NodeType::SERVER:
		std::cout << "SERVER";
		break;
	case AST::NodeType::DIRECTIVE:
		std::cout << "DIRECTIVE: " << node.value;
		break;
	case AST::NodeType::LOCATION:
		std::cout << "LOCATION: " << node.value;
		break;
	case AST::NodeType::ARG:
		std::cout << "ARG: " << node.value;
		break;
	case AST::NodeType::ERROR:
		std::cout << "ERROR: " << node.message;
		break;
	default:
		std::cout << "UNKNOWN";
		break;
	}

	// Print line and column info if available
	if (node.line > 0)
		std::cout << " (line " << node.line << ", col " << node.column << ")";

	std::cout << std::endl;

	// Recursively print children
	for (size_t i = 0; i < node.children.size(); ++i)
	{
		printASTRecursive(*node.children[i], depth + 1);
	}
}

/* ************************************************************************** */