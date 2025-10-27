#ifndef MIMETYPE_RESOLVER_HPP
#define MIMETYPE_RESOLVER_HPP

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

// Static class to resolve mime types from extension or magic bytes
// Singleton nature allows multiple instances to share the same map
class MimeTypeResolver
{
private:
	MimeTypeResolver();
	MimeTypeResolver(MimeTypeResolver const &src);
	MimeTypeResolver &operator=(MimeTypeResolver const &rhs);

	struct MagicRule
	{
		size_t offset;
		std::string pattern;
		std::string mime;
	};

	static std::map<std::string, std::string> &getMimeTypesMap()
	{
		static std::map<std::string, std::string> instance;
		return instance;
	}

	static std::vector<MagicRule> &getMagicRules()
	{
		static std::vector<MagicRule> rules;
		if (rules.empty())
		{
			rules.push_back({0, "\x89PNG\r\n\x1a\n", "image/png"});
			rules.push_back({0, "%PDF-", "application/pdf"});
			rules.push_back({0, "PK\x03\x04", "application/zip"});
			rules.push_back({0, "\xFF\xD8\xFF", "image/jpeg"});
			rules.push_back({0, "GIF87a", "image/gif"});
			rules.push_back({0, "GIF89a", "image/gif"});
		}
		return rules;
	}

	static bool &getInitialized()
	{
		static bool instance = false;
		return instance;
	}

	static std::string &getCurrentFilePathRef()
	{
		static std::string instance;
		return instance;
	}

	static time_t &getLastModifiedRef()
	{
		static time_t instance = 0;
		return instance;
	}

	static bool hasFileChanged()
	{
		if (getCurrentFilePathRef().empty())
			return false;
		struct stat fileStat;
		if (stat(getCurrentFilePathRef().c_str(), &fileStat) != 0)
			return true;
		if (getLastModifiedRef() != fileStat.st_mtime)
		{
			getLastModifiedRef() = fileStat.st_mtime;
			return true;
		}
		return false;
	}

	static void initMimeTypes()
	{
		if (getInitialized() && !hasFileChanged())
			return;
		getInitialized() = true;
		getMimeTypesMap().clear();

		const char *home = getenv("HOME");
		std::string userFile = std::string(home ? home : "") + "/mime.types";
		std::ifstream file(userFile.c_str());
		if (file.is_open())
		{
			getCurrentFilePathRef() = userFile;
		}
		else
		{
			std::string systemFile = "/etc/mime.types";
			file.open(systemFile.c_str());
			if (file.is_open())
			{
				getCurrentFilePathRef() = systemFile;
			}
			else
			{
				throw std::runtime_error("No mime types file found");
			}
		}

		struct stat fileStat;
		if (stat(getCurrentFilePathRef().c_str(), &fileStat) == 0)
		{
			getLastModifiedRef() = fileStat.st_mtime;
		}

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#')
				continue;
			std::stringstream ss(line);
			std::string mimeType, extension;
			ss >> mimeType;
			while (ss >> extension)
			{
				getMimeTypesMap()[extension] = mimeType;
			}
		}
		file.close();
	}

	static bool isLikelyText(const std::vector<char> &buffer)
	{
		for (size_t i = 0; i < buffer.size(); ++i)
		{
			unsigned char c = static_cast<unsigned char>(buffer[i]);
			if (c == 0 || (c < 0x09) || (c > 0x7E && c < 0xA0))
				return false;
		}
		return true;
	}

	static std::string guessMimeTypeFromContent(const std::vector<char> &buffer)
	{
		const std::vector<MagicRule> &rules = getMagicRules();
		for (size_t i = 0; i < rules.size(); ++i)
		{
			const MagicRule &rule = rules[i];
			if (buffer.size() >= rule.offset + rule.pattern.size() &&
				std::memcmp(&buffer[rule.offset], rule.pattern.c_str(), rule.pattern.size()) == 0)
				return rule.mime;
		}
		return isLikelyText(buffer) ? "text/plain" : "application/octet-stream";
	}

public:
	~MimeTypeResolver()
	{
		getMimeTypesMap().clear();
		getInitialized() = false;
		getCurrentFilePathRef().clear();
		getLastModifiedRef() = 0;
	}

	static std::string getMimeType(const std::string &extension)
	{
		initMimeTypes();
		std::map<std::string, std::string>::iterator it = getMimeTypesMap().find(extension);
		if (it != getMimeTypesMap().end())
			return it->second;
		return "application/octet-stream";
	}

	static std::string getMimeTypeForPath(const std::string &path)
	{
		initMimeTypes();
		size_t dot = path.rfind('.');
		if (dot == std::string::npos || dot == path.length() - 1)
			return "application/octet-stream";
		std::string ext = path.substr(dot + 1);
		return getMimeType(ext);
	}

	static std::string resolveMimeType(const std::string &path, const std::vector<char> &buffer)
	{
		std::string mime = getMimeTypeForPath(path);
		if (mime == "application/octet-stream")
			mime = guessMimeTypeFromContent(buffer);
		return mime;
	}

	static void forceReload()
	{
		getInitialized() = false;
		getLastModifiedRef() = 0;
		initMimeTypes();
	}

	static std::string getCurrentFilePath()
	{
		return getCurrentFilePathRef();
	}

	static bool isInitialized()
	{
		return getInitialized();
	}
};

#endif