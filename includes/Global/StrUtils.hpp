#ifndef STRINGUTILS_HPP
#define STRINGUTILS_HPP

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <sstream>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace StrUtils
{
// ============================================================
// STRING MANIPULATION
// ============================================================

std::string toLowerCase(const std::string &str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::tolower);
	return result;
}

std::string toUpperCase(const std::string &str)
{
	std::string result = str;
	std::transform(result.begin(), result.end(), result.begin(), ::toupper);
	return result;
}

template <typename T> std::string toString(T value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}

template <typename T> T fromString(const std::string &str)
{
	T value;
	std::stringstream ss(str);
	ss >> value;
	return value;
}

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
	parts.push_back(current);
	return parts;
}

// ============================================================
// CHARACTER VALIDATION
// ============================================================

// Check if character is a control character (0x00-0x1F or 0x7F)
bool isControlCharacter(unsigned char c)
{
	return (c < 0x20 || c == 0x7F);
}

// Check if character is valid in a path
bool isValidPathCharacter(char c)
{
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/' || c == '.' ||
		   c == '-' || c == '_';
}

int hexCharToInt(char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

// ============================================================
// STRING VALIDATION & ANALYSIS
// ============================================================

// Check if string contains control characters
bool hasControlCharacters(const std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (isControlCharacter(static_cast<unsigned char>(str[i])))
			return true;
	}
	return false;
}

// Check if string contains only printable ASCII (0x20-0x7E)
bool isPrintableAscii(const std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(str[i]);
		if (c < 0x20 || c > 0x7E)
			return false;
	}
	return true;
}

// Check if string has consecutive dots
bool hasConsecutiveDots(const std::string &str)
{
	for (size_t i = 0; i < str.length() - 1; ++i)
	{
		if (str[i] == '.' && str[i + 1] == '.')
			return true;
	}
	return false;
}

// Find first control character position
size_t findControlCharacter(const std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (isControlCharacter(static_cast<unsigned char>(str[i])))
			return i;
	}
	return std::string::npos;
}

// Get human-readable representation of control character
std::string controlCharToString(unsigned char c)
{
	if (!isControlCharacter(c))
	{
		std::string result;
		result += c;
		return result;
	}

	switch (c)
	{
	case 0x00:
		return "\\0 (NUL)";
	case 0x07:
		return "\\a (BEL)";
	case 0x08:
		return "\\b (BS)";
	case 0x09:
		return "\\t (TAB)";
	case 0x0A:
		return "\\n (LF)";
	case 0x0B:
		return "\\v (VT)";
	case 0x0C:
		return "\\f (FF)";
	case 0x0D:
		return "\\r (CR)";
	case 0x7F:
		return "\\x7F (DEL)";
	default:
	{
		std::stringstream ss;
		ss << "\\x" << std::hex << std::uppercase;
		ss.width(2);
		ss.fill('0');
		ss << static_cast<int>(c);
		return ss.str();
	}
	}
}

// Remove control characters from string
std::string removeControlCharacters(const std::string &str)
{
	std::string result;
	result.reserve(str.size());
	for (size_t i = 0; i < str.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(str[i]);
		if (!isControlCharacter(c))
			result += c;
	}
	return result;
}

// ============================================================
// URL/PATH ENCODING & NORMALIZATION
// ============================================================

// Decode percent-encoded URL string
std::string percentDecode(const std::string &input)
{
	std::string output;
	output.reserve(input.length()); // Optimize memory allocation

	for (size_t i = 0; i < input.length(); ++i)
	{
		if (input[i] == '%')
		{
			// Check if we have enough characters for %XX
			if (i + 2 < input.length())
			{
				const char c1 = input[i + 1];
				const char c2 = input[i + 2];

				// Validate hex digits (C++98 compatible)
				if (std::isxdigit(static_cast<unsigned char>(c1)) && std::isxdigit(static_cast<unsigned char>(c2)))
				{
					// Convert hex to decimal
					char hex[3] = {c1, c2, '\0'};
					char *endPtr = NULL;
					long value = std::strtol(hex, &endPtr, 16);

					if (endPtr && *endPtr == '\0') // Successful conversion
					{
						output += static_cast<char>(value);
						i += 2; // Skip the two hex digits
						continue;
					}
				}
			}
			// Invalid percent encoding - keep the '%' literally
			output += input[i];
		}
		else if (input[i] == '+')
		{
			// Query string convention: '+' represents space
			output += ' ';
		}
		else
		{
			output += input[i];
		}
	}

	return output;
}

// Normalize multiple slashes to single slash
std::string normalizeSlashes(const std::string &path)
{
	std::string result;
	result.reserve(path.length());

	bool lastWasSlash = false;
	for (size_t i = 0; i < path.length(); ++i)
	{
		if (path[i] == '/')
		{
			if (!lastWasSlash)
			{
				result += '/';
				lastWasSlash = true;
			}
		}
		else
		{
			result += path[i];
			lastWasSlash = false;
		}
	}
	return result;
}

// Remove dot segments (. and ..) from path
std::string removeDotSegments(const std::string &path)
{
	std::vector<std::string> segments;
	std::vector<std::string> parts = splitString(path, '/');

	for (size_t i = 0; i < parts.size(); ++i)
	{
		const std::string &segment = parts[i];

		if (segment.empty() || segment == ".")
		{
			continue;
		}
		else if (segment == "..")
		{
			if (!segments.empty())
				segments.pop_back();
		}
		else
		{
			segments.push_back(segment);
		}
	}

	if (segments.empty())
		return "/";

	std::string result;
	for (size_t i = 0; i < segments.size(); ++i)
	{
		result += "/";
		result += segments[i];
	}
	return result;
}

// Trim trailing slashes from path
std::string trimTrailingSlashes(const std::string &path)
{
	if (path.empty())
		return path;

	size_t end = path.find_last_not_of('/');
	if (end == std::string::npos)
		return "/";

	return path.substr(0, end + 1);
}

// Full URI path sanitization (decode, normalize, remove traversal)
std::string sanitizeUriPath(const std::string &path)
{
	std::string decoded = percentDecode(path);
	std::string normalized = normalizeSlashes(decoded);
	std::string clean = removeDotSegments(normalized);

	if (clean.empty() || clean[0] != '/')
		clean = "/" + clean;

	return clean;
}

// Check if sanitized path is safe
bool isSafePath(const std::string &path)
{
	if (path.find("..") != std::string::npos)
		return false;
	if (path.find("//") != std::string::npos)
		return false;
	if (path.empty() || path[0] != '/')
		return false;
	return true;
}

bool hasSpaces(const std::string &str)
{
	for (size_t i = 0; i < str.size(); ++i)
	{
		if (isspace(str[i]))
			return true;
	}
	return false;
}

// ============================================================
// FILESYSTEM OPERATIONS
// ============================================================

// Basic existence checks
bool pathExists(const std::string &path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0;
}

bool pathExistsNoFollow(const std::string &path)
{
	struct stat st;
	return lstat(path.c_str(), &st) == 0;
}

// Type checks
bool isDirectory(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISDIR(st.st_mode);
}

bool isFile(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return false;
	return S_ISREG(st.st_mode);
}

bool isSymbolicLink(const std::string &path)
{
	struct stat st;
	if (lstat(path.c_str(), &st) != 0)
		return false;
	return S_ISLNK(st.st_mode);
}

// Permission checks
bool isReadable(const std::string &path)
{
	return access(path.c_str(), R_OK) == 0;
}

bool isWritable(const std::string &path)
{
	return access(path.c_str(), W_OK) == 0;
}

bool isExecutable(const std::string &path)
{
	return access(path.c_str(), X_OK) == 0;
}

// Path properties
bool isAbsolutePath(const std::string &path)
{
	return !path.empty() && path[0] == '/';
}

long getFileSize(const std::string &path)
{
	struct stat st;
	if (stat(path.c_str(), &st) != 0)
		return -1;
	return static_cast<long>(st.st_size);
}

// Path resolution
std::string getCanonicalPath(const std::string &path)
{
	char *resolved = realpath(path.c_str(), NULL);
	if (resolved == NULL)
		return "";

	std::string result(resolved);
	free(resolved);
	return result;
}

// Security: Check if path is within allowed directory
bool isPathWithinDirectory(const std::string &path, const std::string &allowedDir)
{
	std::string canonicalPath = getCanonicalPath(path);
	std::string canonicalDir = getCanonicalPath(allowedDir);

	if (canonicalPath.empty() || canonicalDir.empty())
		return false;

	if (!canonicalDir.empty() && canonicalDir[canonicalDir.length() - 1] != '/')
		canonicalDir += '/';

	return canonicalPath.compare(0, canonicalDir.length(), canonicalDir) == 0;
}

// ============================================================
// HIGH-LEVEL VALIDATION (Returns error messages)
// ============================================================

// Validate no control characters
std::string validateNoControlCharacters(const std::string &str, const std::string &fieldName)
{
	size_t pos = findControlCharacter(str);
	if (pos == std::string::npos)
		return "";

	unsigned char c = static_cast<unsigned char>(str[pos]);
	std::stringstream ss;
	ss << fieldName << ": Control character " << controlCharToString(c) << " found at position " << pos
	   << " in: " << str;
	return ss.str();
}

// Validate directory path (exists, readable)
std::string validateDirectoryPath(const std::string &path, const std::string &directiveName)
{
	if (path.empty())
		return directiveName + ": Path cannot be empty";

	if (hasSpaces(path))
		return directiveName + ": Path contains spaces";

	if (hasControlCharacters(path))
		return directiveName + ": Path contains control characters";

	if (hasConsecutiveDots(path))
		return directiveName + ": Path contains consecutive dots";

	if (!isAbsolutePath(path))
		return directiveName + ": Path must be absolute (start with /): " + path;

	if (!pathExists(path))
		return directiveName + ": Directory does not exist: " + path;

	if (!isDirectory(path))
		return directiveName + ": Path is not a directory: " + path;

	if (!isReadable(path))
		return directiveName + ": Directory is not readable: " + path;

	return "";
}

// Validate file path (exists, readable)
std::string validateFilePath(const std::string &path, const std::string &directiveName)
{
	if (path.empty())
		return directiveName + ": Path cannot be empty";

	if (hasSpaces(path))
		return directiveName + ": Path contains spaces";

	if (hasControlCharacters(path))
		return directiveName + ": Path contains control characters";

	if (hasConsecutiveDots(path))
		return directiveName + ": Path contains consecutive dots";

	if (!isAbsolutePath(path))
		return directiveName + ": Path must be absolute (start with /): " + path;

	if (!pathExists(path))
		return directiveName + ": File does not exist: " + path;

	if (!isFile(path))
		return directiveName + ": Path is not a regular file: " + path;

	if (!isReadable(path))
		return directiveName + ": File is not readable: " + path;

	return "";
}

} // namespace StrUtils

#endif // STRINGUTILS_HPP