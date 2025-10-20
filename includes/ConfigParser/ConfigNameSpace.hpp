#ifndef CONFIG_NAMESPACE_HPP
#define CONFIG_NAMESPACE_HPP

#include "../../includes/Global/Logger.hpp"
#include <cstddef>
#include <cstring>
#include <limits.h>
#include <netinet/in.h>
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

enum NodeType
{
	UNKNOWN = 0,
	CONFIG = 1,
	SERVER = 2,
	DIRECTIVE = 3,
	LOCATION = 4,
	ARG = 5,
	ERROR = 6
};

struct ASTNode
{
	NodeType type;
	std::string value;
	size_t line;
	size_t column;
	std::string message; // For errors;
	std::vector<ASTNode *> children;

	// Constructor
	ASTNode(NodeType t, const std::string &v = "") : type(t), value(v)
	{
	}

	// Destructor cleans up children
	~ASTNode()
	{
		for (size_t i = 0; i < children.size(); ++i)
		{
			delete children[i];
		}
	}

	// Utility
	void addChild(ASTNode *child)
	{
		children.push_back(child);
	}

	void Error(std::string message)
	{
		this->type = NodeType::ERROR;
		this->message = message;
	}
};

namespace IPAddressParser
{
namespace Utils
{
// Helper function to convert string to integer (replaces atoi)
long stringToLong(const std::string &str, bool &isValid)
{
	isValid = true;
	if (str.empty())
	{
		isValid = false;
		return 0;
	}

	long result = 0;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] < '0' || str[i] > '9')
		{
			isValid = false;
			return 0;
		}

		// Check for overflow before multiplying
		if (result > (LONG_MAX - (str[i] - '0')) / 10)
		{
			isValid = false;
			return 0;
		}

		result = result * 10 + (str[i] - '0');
	}
	return result;
}

// Split string by delimiter (replaces strtok)
std::vector<std::string> splitString(const std::string &str, char delimiter)
{
	std::vector<std::string> parts;
	std::string current;

	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == delimiter)
		{
			parts.push_back(current);
			current.clear();
		}
		else
		{
			current += str[i];
		}
	}
	parts.push_back(current); // Add the last part
	return parts;
}
} // namespace Utils
// Parse IPv4 address string to 32-bit integer (network byte order)
bool parseIPv4(const std::string &ipStr, uint32_t &result)
{
	// Handle special cases
	if (ipStr.empty())
		return false;
	if (ipStr == "0.0.0.0")
	{
		result = 0; // INADDR_ANY in network byte order
		return true;
	}

	// Split by dots
	std::vector<std::string> octets = Utils::splitString(ipStr, '.');
	if (octets.size() != 4)
	{
		return false;
	}

	uint32_t addr = 0;
	for (int i = 0; i < 4; ++i)
	{
		bool isValid;
		long octet = Utils::stringToLong(octets[i], isValid);

		if (!isValid || octet < 0 || octet > 255)
		{
			return false;
		}

		// Build address in network byte order (big-endian)
		addr |= (static_cast<uint32_t>(octet) << (24 - i * 8));
	}

	result = htonl(addr); // Convert to network byte order
	return true;
}

// Parse IPv6 address string (basic implementation)
bool parseIPv6(const std::string &ipStr, struct in6_addr &result)
{
	// Handle special cases
	if (ipStr.empty())
		return false;
	if (ipStr == "::")
	{
		std::memset(&result, 0, sizeof(result));
		return true;
	}

	// This is a simplified IPv6 parser
	// A full implementation would handle :: compression, mixed notation,
	// etc.
	std::vector<std::string> parts = Utils::splitString(ipStr, ':');

	// Basic validation - IPv6 has 8 16-bit parts
	if (parts.size() > 8)
		return false;

	// Clear result
	std::memset(&result, 0, sizeof(result));

	// Parse each part as hexadecimal
	for (size_t i = 0; i < parts.size() && i < 8; ++i)
	{
		if (parts[i].empty())
			continue; // Handle :: notation

		// Parse hexadecimal string manually
		uint16_t value = 0;
		for (size_t j = 0; j < parts[i].length(); ++j)
		{
			char c = parts[i][j];
			int digit;

			if (c >= '0' && c <= '9')
			{
				digit = c - '0';
			}
			else if (c >= 'a' && c <= 'f')
			{
				digit = c - 'a' + 10;
			}
			else if (c >= 'A' && c <= 'F')
			{
				digit = c - 'A' + 10;
			}
			else
			{
				return false; // Invalid character
			}

			value = value * 16 + digit;
		}

		// Store in network byte order
		result.s6_addr[i * 2] = (value >> 8) & 0xFF;
		result.s6_addr[i * 2 + 1] = value & 0xFF;
	}

	return true;
}

// Convert 32-bit integer to IPv4 string (from network byte order)
std::string ipv4ToString(uint32_t addr)
{
	// Convert from network byte order
	addr = ntohl(addr);

	std::string result;

	// Extract each octet
	for (int i = 3; i >= 0; --i)
	{
		uint32_t octet = (addr >> (i * 8)) & 0xFF;

		// Convert number to string manually
		if (octet == 0)
		{
			result += "0";
		}
		else
		{
			std::string octetStr;
			while (octet > 0)
			{
				octetStr = static_cast<char>('0' + (octet % 10)) + octetStr;
				octet /= 10;
			}
			result += octetStr;
		}

		if (i > 0)
			result += ".";
	}

	return result;
}

// Convert IPv6 binary to string (basic implementation)
std::string ipv6ToString(const struct in6_addr &addr)
{
	std::string result;

	for (int i = 0; i < 8; ++i)
	{
		if (i > 0)
			result += ":";

		uint16_t part = (addr.s6_addr[i * 2] << 8) | addr.s6_addr[i * 2 + 1];

		// Convert to hex string manually
		if (part == 0)
		{
			result += "0";
		}
		else
		{
			std::string hexStr;
			while (part > 0)
			{
				int digit = part % 16;
				char c = (digit < 10) ? ('0' + digit) : ('a' + digit - 10);
				hexStr = c + hexStr;
				part /= 16;
			}
			result += hexStr;
		}
	}

	return result;
}

// Validate if string looks like IPv4
bool looksLikeIPv4(const std::string &str)
{
	if (str.empty())
		return false;

	int dotCount = 0;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (str[i] == '.')
		{
			dotCount++;
		}
		else if (str[i] < '0' || str[i] > '9')
		{
			return false;
		}
	}
	return dotCount == 3;
}

// Validate if string looks like IPv6
bool looksLikeIPv6(const std::string &str)
{
	if (str.empty())
		return false;
	if (str.find(':') == std::string::npos)
		return false;

	// Basic IPv6 format check
	for (size_t i = 0; i < str.length(); ++i)
	{
		char c = str[i];
		if (!(c == ':' || (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
		{
			return false;
		}
	}
	return true;
}

bool parseListenDirective(const std::string &directive, uint32_t &ipv4, struct in6_addr &ipv6, uint16_t &port,
						  bool &isIPv6)
{
	std::string ipPart, portPart;
	size_t colonPos = directive.rfind(':');

	if (colonPos == std::string::npos)
		return false; // No port specified

	ipPart = directive.substr(0, colonPos);
	portPart = directive.substr(colonPos + 1);

	// Strip brackets for IPv6
	if (!ipPart.empty() && ipPart.front() == '[' && ipPart.back() == ']')
		ipPart = ipPart.substr(1, ipPart.length() - 2);

	// Validate port
	bool portValid;
	long portNum = Utils::stringToLong(portPart, portValid);
	if (!portValid || portNum < 1 || portNum > 65535)
		return false;
	port = static_cast<uint16_t>(portNum);

	// Handle wildcard
	if (ipPart == "*" || ipPart == "0.0.0.0")
	{
		ipv4 = 0;
		isIPv6 = false;
		return true;
	}
	if (ipPart == "::")
	{
		std::memset(&ipv6, 0, sizeof(ipv6));
		isIPv6 = true;
		return true;
	}

	// Try IPv4
	if (looksLikeIPv4(ipPart))
	{
		if (!parseIPv4(ipPart, ipv4))
			return false;
		isIPv6 = false;
		return true;
	}

	// Try IPv6
	if (looksLikeIPv6(ipPart))
	{
		if (!parseIPv6(ipPart, ipv6))
			return false;
		isIPv6 = true;
		return true;
	}

	return false; // Unknown format
}
} // namespace IPAddressParser

// Function to merge directives in server blocks and location blocks
namespace ConfigMerger
{
namespace Utils
{

void mergeLocationBlock(AST::ASTNode &LocationBlock)
{
	for (std::vector<AST::ASTNode *>::iterator it = LocationBlock.children.begin(); it != LocationBlock.children.end();
		 ++it)
	{
		switch ((*it)->type)
		{
		case AST::NodeType::DIRECTIVE:
		{
			for (std::vector<AST::ASTNode *>::iterator it2 = it + 1; it2 != LocationBlock.children.end(); ++it2)
			{
				if ((*it2)->type == AST::NodeType::DIRECTIVE && (*it2)->value == (*it)->value)
				{
					(*it)->children.insert((*it)->children.end(), (*it2)->children.begin(), (*it2)->children.end());
					it2 = LocationBlock.children.erase(it2);
				}
			}
			break;
		default:
			(*it)->Error("Unexpected token in location block");
			break;
		}
		}
	}
}

void mergeServerDirectives(AST::ASTNode &ServerBlock)
{
	for (std::vector<AST::ASTNode *>::iterator it = ServerBlock.children.begin(); it != ServerBlock.children.end();
		 ++it)
	{
		switch ((*it)->type)
		{
		case AST::NodeType::DIRECTIVE:
		{
			for (std::vector<AST::ASTNode *>::iterator it2 = it + 1; it2 != ServerBlock.children.end(); ++it2)
			{
				if ((*it2)->type == AST::NodeType::DIRECTIVE && (*it2)->value == (*it)->value)
				{
					(*it)->children.insert((*it)->children.end(), (*it2)->children.begin(), (*it2)->children.end());
					it2 = ServerBlock.children.erase(it2);
					break;
				}
			}
			break;
		}
		case AST::NodeType::LOCATION:
		{
			Utils::mergeLocationBlock(**it);
		}
		break;
		default:
			(*it)->Error("Unexpected token in server block");
			break;
		}
	}
}

// Recursively trim errors from the AST
void trimErrors(AST::ASTNode &head)
{
	for (std::vector<AST::ASTNode *>::iterator it = head.children.begin(); it != head.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::ERROR)
		{
			std::stringstream ss;
			ss << (*it)->message << " at line " << (*it)->line << ":" << (*it)->column;
			Logger::log(Logger::ERROR, ss.str());
			head.children.erase(it);
		}
		else
		{
			trimErrors(**it);
		}
	}
}

} // namespace Utils

void merge(AST::ASTNode &head)
{
	for (std::vector<AST::ASTNode *>::iterator it = head.children.begin(); it != head.children.end(); ++it)
	{
		if ((*it)->type == AST::NodeType::SERVER)
		{
			Utils::mergeServerDirectives(**it);
		}
		else
		{
			(*it)->Error("Unexpected token in server block");
		}
	}
	Utils::trimErrors(head);
}
} // namespace ConfigMerger
} // namespace AST

#endif
