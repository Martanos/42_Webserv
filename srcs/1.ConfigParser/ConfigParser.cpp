<<<<<<< HEAD
#include "../../includes/ConfigParser.hpp"
#include "../../includes/Location.hpp"
#include "../../includes/StringUtils.hpp"
#include "../../includes/ConfigUtils.hpp"
=======
#include "../../includes/ConfigParser/ConfigParser.hpp"
#include <sstream>
#include <stdexcept>
>>>>>>> ConfigParserRefactor

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
<<<<<<< HEAD
	Logger::debug("ConfigParser: Starting to parse configuration file: " + filename);
	std::ifstream file(filename.c_str());
	std::stringstream errorMessage;
	if (!file.is_open())
=======
	Token::Token open = expect(Token::TOKEN_OPEN_BRACE, "'{' after server");
	AST::ASTNode srv(AST::NodeType::SERVER);
	srv.line = open.line;
	srv.column = open.column;

	while (true)
>>>>>>> ConfigParserRefactor
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

<<<<<<< HEAD
	std::stringstream buffer;
	buffer << file.rdbuf(); // Load entire file into stringstream for efficient
							// parsing
	file.close();
	Logger::debug("ConfigParser: Configuration file loaded successfully");

	std::string line;
	bool foundHttp = false;
	bool insideHttp = false;
	bool insideServer = false;
	double lineNumber = 0; // Initialize line number for error reporting
	while (std::getline(buffer, line))
	{
		lineNumber++;
		if (line.empty() || line[0] == '#')
		{
			continue; // Skip empty lines and comments
		}
		line = StringUtils::trim(line);

		httpblockcheck(line, foundHttp, insideHttp);
		if (serverblockcheck(line, insideHttp, insideServer))
		{
			_parseServerBlock(buffer, lineNumber); // Handle server block start
		}

		// Handle closing brace for http block (when not inside server)
		if (insideHttp && !insideServer && line == "}")
		{
			insideHttp = false;
		}
	}
	if (_serverConfigs.empty())
	{
		errorMessage << "No valid server blocks found, terminating parser";
		Logger::log(Logger::ERROR, errorMessage.str());
		throw std::runtime_error(errorMessage.str());
	}
	Logger::debug("ConfigParser: Successfully parsed " + StringUtils::toString(_serverConfigs.size()) + " server configurations");
	return true;
=======
	cfg.addChild(&srv);
>>>>>>> ConfigParserRefactor
}

AST::ASTNode ConfigParser::parseDirective()
{
	Token::Token name = expect(Token::TOKEN_IDENTIFIER, "directive name");
	AST::ASTNode d(AST::NodeType::DIRECTIVE);
	d.value = name.lexeme;
	d.line = name.line;
	d.column = name.column;
	size_t position = 0;

	// gather args until ';'
	while (true)
	{
		position++;
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
			directiveArg.position = position;
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
<<<<<<< HEAD
	return false; // Not a http block directive
=======
	return loc;
>>>>>>> ConfigParserRefactor
}

/*
** --------------------------------- METHODS ----------------------------------
*/
AST::ASTNode ConfigParser::parse()
{
	AST::ASTNode cfg(AST::NodeType::CONFIG);
	while (true)
	{
<<<<<<< HEAD
		line = StringUtils::trim(line);
		lineNumber++;

		if (line.empty() || line[0] == '#')
		{
			continue; // Skip empty lines and comments
		}

		if (line == "}")
		{
			break; // End of server block
		}

		// Basic server directive parsing - can be expanded later
		std::vector<std::string> tokens = StringUtils::split(line);
		if (tokens.empty())
		{
			continue; // Skip empty lines
		}
		std::string directive = tokens[0];

		switch (_getServerDirectiveType(directive))
		{
		case SERVER_LISTEN:
			currentServer.addHosts_ports(line, lineNumber);
			break;
		case SERVER_NAME:
			currentServer.addServerName(line, lineNumber);
			break;
		case SERVER_ROOT:
			currentServer.addRoot(line, lineNumber);
			break;
		case SERVER_INDEX:
			currentServer.addIndexes(line, lineNumber);
			break;
		case SERVER_ERROR_PAGE:
			currentServer.addStatusPages(line, lineNumber);
			break;
		case SERVER_MAX_URI_SIZE:
			currentServer.addMaxUriSize(line, lineNumber);
			break;
		case SERVER_MAX_HEADER_SIZE:
			currentServer.addMaxHeaderSize(line, lineNumber);
			break;
		case SERVER_CLIENT_MAX_BODY_SIZE:
			currentServer.addClientMaxBodySize(line, lineNumber);
			break;
		case SERVER_AUTOINDEX:
			currentServer.addAutoindex(line, lineNumber);
			break;
		case SERVER_ACCESS_LOG:
			currentServer.addAccessLog(line, lineNumber);
			break;
		case SERVER_ERROR_LOG:
			currentServer.addErrorLog(line, lineNumber);
			break;
		case SERVER_KEEP_ALIVE:
			currentServer.addKeepAlive(line, lineNumber);
			break;
		case SERVER_LOCATION:
			_parseLocationBlock(buffer, lineNumber, line, currentServer);
			break;
		case SERVER_UNKNOWN:
		default:
			Logger::log(Logger::WARNING, "Unknown server directive: " + directive);
=======
		Token::Token t = _tok->peek(1);
		if (t.type == Token::TOKEN_EOF)
		{
			_tok->nextToken();
>>>>>>> ConfigParserRefactor
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

<<<<<<< HEAD

=======
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
>>>>>>> ConfigParserRefactor

	std::cout << std::endl;

	// Recursively print children
	for (size_t i = 0; i < node.children.size(); ++i)
	{
		printASTRecursive(*node.children[i], depth + 1);
	}
}

<<<<<<< HEAD
ConfigParser::ServerDirectiveType ConfigParser::_getServerDirectiveType(const std::string &directive) const
{
	if (directive == "listen")
		return SERVER_LISTEN;
	if (directive == "server_name")
		return SERVER_NAME;
	if (directive == "root")
		return SERVER_ROOT;
	if (directive == "index")
		return SERVER_INDEX;
	if (directive == "error_page")
		return SERVER_ERROR_PAGE;
	if (directive == "client_max_body_size")
		return SERVER_CLIENT_MAX_BODY_SIZE;
	if (directive == "autoindex")
		return SERVER_AUTOINDEX;
	if (directive == "access_log")
		return SERVER_ACCESS_LOG;
	if (directive == "error_log")
		return SERVER_ERROR_LOG;
	if (directive == "keep_alive")
		return SERVER_KEEP_ALIVE;
	if (directive == "location")
		return SERVER_LOCATION;
	return SERVER_UNKNOWN;
}

ConfigParser::LocationDirectiveType ConfigParser::_getLocationDirectiveType(const std::string &directive) const
{
	if (directive == "root")
		return LOCATION_ROOT;
	if (directive == "allowed_methods")
		return LOCATION_ACCEPTED_HTTP_METHODS;
	if (directive == "return")
		return LOCATION_RETURN;
	if (directive == "autoindex")
		return LOCATION_AUTOINDEX;
	if (directive == "index")
		return LOCATION_INDEX;
	if (directive == "cgi_path")
		return LOCATION_CGI_PATH;
	if (directive == "cgi_param")
		return LOCATION_CGI_PARAM;
	if (directive == "upload_path")
		return LOCATION_UPLOAD_PATH;
	return LOCATION_UNKNOWN;
}

void ConfigParser::_parseLocationBlock(std::stringstream &buffer, double &lineNumber, const std::string &locationLine,
									   ServerConfig &currentServer)
{
	Location currentLocation;
	std::string line;

	// Parse the location directive line first to extract the path
	std::string locationDirective = locationLine + ";"; // Add semicolon for validation
	currentLocation.addPath(locationDirective, lineNumber);

	while (std::getline(buffer, line))
	{
		line = StringUtils::trim(line);
		lineNumber++;

		if (line.empty() || line[0] == '#')
		{
			continue; // Skip empty lines and comments
		}

		if (line == "}")
		{
			break; // End of location block
		}

		// Parse location directive
		std::vector<std::string> tokens = StringUtils::split(line);
		if (tokens.empty())
		{
			continue; // Skip empty lines
		}
		std::string directive = tokens[0];

		switch (_getLocationDirectiveType(directive))
		{
		case LOCATION_ROOT:
			currentLocation.addRoot(line, lineNumber);
			break;
		case LOCATION_ACCEPTED_HTTP_METHODS:
			currentLocation.addAllowedMethods(line, lineNumber);
			break;
		case LOCATION_RETURN:
			currentLocation.addRedirect(line, lineNumber);
			break;
		case LOCATION_AUTOINDEX:
			currentLocation.addAutoIndex(line, lineNumber);
			break;
		case LOCATION_CGI_PATH:
			currentLocation.addCgiPath(line, lineNumber);
			break;
		case LOCATION_CGI_PARAM:
			currentLocation.addCgiParam(line, lineNumber);
			break;
		case LOCATION_UPLOAD_PATH:
			currentLocation.addUploadPath(line, lineNumber);
			break;
		case LOCATION_UNKNOWN:
		default:
			Logger::log(Logger::WARNING, "Unknown location directive: " + directive);
			break;
		}
	}

	// Add the parsed location to the current server
	// check that a duplicate location is not added
	if (!currentServer.hasLocation(currentLocation))
	{
		currentServer.addLocation(currentLocation, lineNumber);
	}
	else
	{
		Logger::log(Logger::WARNING, "Duplicate location found: " + currentLocation.getPath());
	}
}

const std::vector<ServerConfig>& ConfigParser::getServerConfigs() const
{
	return _serverConfigs;
}
=======
/* ************************************************************************** */
>>>>>>> ConfigParserRefactor
