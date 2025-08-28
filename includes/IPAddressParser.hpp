#ifndef IPADDRESSPARSER_HPP
#define IPADDRESSPARSER_HPP

#include <iostream>
#include <string>
#include <netdb.h>
#include <netinet/in.h>
#include <limits.h>
#include <cstring>
#include <vector>

class IPAddressParser
{
private:
	// Non-instantiable utility class
	IPAddressParser();

	// Helper function to convert string to integer (replaces atoi)
	static long stringToLong(const std::string &str, bool &isValid)
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
	static std::vector<std::string> splitString(const std::string &str, char delimiter)
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

public:
	// Parse IPv4 address string to 32-bit integer (network byte order)
	static bool parseIPv4(const std::string &ipStr, uint32_t &result)
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
		std::vector<std::string> octets = splitString(ipStr, '.');
		if (octets.size() != 4)
		{
			return false;
		}

		uint32_t addr = 0;
		for (int i = 0; i < 4; ++i)
		{
			bool isValid;
			long octet = stringToLong(octets[i], isValid);

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
	static bool parseIPv6(const std::string &ipStr, struct in6_addr &result)
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
		// A full implementation would handle :: compression, mixed notation, etc.
		std::vector<std::string> parts = splitString(ipStr, ':');

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
	static std::string ipv4ToString(uint32_t addr)
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
	static std::string ipv6ToString(const struct in6_addr &addr)
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
	static bool looksLikeIPv4(const std::string &str)
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
	static bool looksLikeIPv6(const std::string &str)
	{
		if (str.empty())
			return false;
		if (str.find(':') == std::string::npos)
			return false;

		// Basic IPv6 format check
		for (size_t i = 0; i < str.length(); ++i)
		{
			char c = str[i];
			if (!(c == ':' || (c >= '0' && c <= '9') ||
				  (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')))
			{
				return false;
			}
		}
		return true;
	}
};

#endif /* ************************************************* IPADDRESSPARSER_H */
