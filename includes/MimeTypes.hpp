#ifndef MIMETYPES_HPP
#define MIMETYPES_HPP

#include <Logger.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <sys/stat.h>

class MimeTypes
{
private:
	// OOC stuff
	MimeTypes();
	MimeTypes(MimeTypes const &src);
	MimeTypes &operator=(MimeTypes const &rhs)
	{
		if (this != &rhs)
		{
			getMimeTypesMap() = rhs.getMimeTypesMap();
			getInitialized() = rhs.getInitialized();
			getCurrentFilePathRef() = rhs.getCurrentFilePathRef();
			getLastModifiedRef() = rhs.getLastModifiedRef();
		}
		return *this;
	}

	static std::map<std::string, std::string> &getMimeTypesMap()
	{
		static std::map<std::string, std::string> instance;
		return (instance);
	}

	static bool &getInitialized()
	{
		static bool instance = false;
		return (instance);
	}

	static std::string &getCurrentFilePathRef()
	{
		static std::string instance;
		return (instance);
	}

	static time_t &getLastModifiedRef()
	{
		static time_t instance = 0;
		return (instance);
	}

	static bool hasFileChanged()
	{
		if (getCurrentFilePathRef().empty())
			return (false);
		struct stat fileStat;
		if (stat(getCurrentFilePathRef().c_str(), &fileStat) != 0)
			return (true);
		if (getLastModifiedRef() != fileStat.st_mtime)
		{
			getLastModifiedRef() = fileStat.st_mtime;
			return (true);
		}
		return (false);
	}

	static void initMimeTypes()
	{
		if (getInitialized() && !hasFileChanged())
			return;
		getInitialized() = true;
		getMimeTypesMap().clear();
		std::string userFile = "~/mime.types";
		std::ifstream file(userFile.c_str());
		if (file.is_open())
		{
			getCurrentFilePathRef() = userFile;
		}
		else
		{
			Logger::log(Logger::WARNING,
						"User mime types file not defined using system mime types");
			std::string systemFile = "/etc/mime.types";
			file.open(systemFile.c_str());
			if (file.is_open())
			{
				getCurrentFilePathRef() = systemFile;
			}
			else
			{
				Logger::log(Logger::ERROR, "System mime types file not found");
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
			std::string mimeType;
			std::string extension;
			ss >> mimeType;
			while (ss >> extension)
			{
				getMimeTypesMap()[extension] = mimeType;
				Logger::log(Logger::DEBUG, "Added mime type: " + mimeType + " with extension: " + extension);
			}
		}
		file.close();
		{
			std::stringstream ss;
			ss << "Loaded " << getMimeTypesMap().size() << " mime types from " << getCurrentFilePathRef();
			Logger::log(Logger::INFO, ss.str());
		}
	}

public:
	~MimeTypes()
	{
		getMimeTypesMap().clear();
		getInitialized() = false;
		getCurrentFilePathRef().clear();
		getLastModifiedRef() = 0;
	}

	// Returns mime type for extension, checks for file changes automatically
	static std::string getMimeType(const std::string &extension)
	{
		initMimeTypes();
		std::map<std::string,
				 std::string>::iterator it = getMimeTypesMap().find(extension);
		if (it != getMimeTypesMap().end())
		{
			return (it->second);
		}
		return ("");
	}

	// Force reload regardless of file timestamp
	static void forceReload()
	{
		getInitialized() = false;
		getLastModifiedRef() = 0;
		initMimeTypes();
	}

	static std::string getCurrentFilePath()
	{
		return (getCurrentFilePathRef());
	}

	static bool isInitialized()
	{
		return (getInitialized());
	}
};

#endif

/* ******************************************************* MIMETYPES_HPP */
