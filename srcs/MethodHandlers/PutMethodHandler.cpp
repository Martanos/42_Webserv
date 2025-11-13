#include "../../includes/Core/PutMethodHandler.hpp"
#include <cerrno>
#include <cstring>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

PutMethodHandler::PutMethodHandler()
{
}

PutMethodHandler::PutMethodHandler(const PutMethodHandler &other)
{
	(void)other;
}

PutMethodHandler::~PutMethodHandler()
{
}

PutMethodHandler &PutMethodHandler::operator=(const PutMethodHandler &other)
{
	(void)other;
	return *this;
}

bool PutMethodHandler::handleRequest(const HttpRequest &request, HttpResponse &response, const Server *server,
									 const Location *location)
{
	(void)server;
	(void)location;
	if (!canHandle(request.getMethod()))
	{
		response.setStatus(405, "Method Not Allowed");
		return false;
	}

	std::string filePath = request.getUri();
	if (filePath.empty() || filePath[0] != '/')
	{
		std::string baseRoot;
		if (location && location->hasRoot())
			baseRoot = location->getRoot();
		else if (server)
			baseRoot = server->getRootPath();

		if (baseRoot.empty())
		{
			Logger::log(Logger::ERROR, "PutMethodHandler: Unable to determine base root for PUT request");
			response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
			return false;
		}

		std::string rawPath = request.getRawUri();
		size_t queryPos = rawPath.find('?');
		if (queryPos != std::string::npos)
			rawPath = rawPath.substr(0, queryPos);
		if (!rawPath.empty() && rawPath[0] == '/')
			rawPath.erase(0, 1);
		if (!baseRoot.empty() && baseRoot[baseRoot.size() - 1] != '/')
			baseRoot += "/";
		filePath = baseRoot + rawPath;
	}

	Logger::debug("PutMethodHandler: Writing to path: " + filePath, __FILE__, __LINE__, __PRETTY_FUNCTION__);

	struct stat st;
	bool existed = (stat(filePath.c_str(), &st) == 0);

	if (!_ensureDirectory(filePath))
	{
		Logger::log(Logger::ERROR, "PutMethodHandler: Failed to ensure directory for path: " + filePath);
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		return false;
	}

	if (!_writeBodyToFile(request, filePath))
	{
		Logger::log(Logger::ERROR, "PutMethodHandler: Failed to write file: " + filePath);
		response.setResponseDefaultBody(500, "Internal Server Error", server, location, HttpResponse::ERROR);
		return false;
	}

	if (existed)
	{
		response.setResponseCustomBody(204, "No Content", "", "text/plain", HttpResponse::SUCCESS);
	}
	else
	{
		response.setResponseCustomBody(201, "Created", "", "text/plain", HttpResponse::SUCCESS);
	}

	Logger::debug("PutMethodHandler: Successfully handled PUT for path: " + filePath, __FILE__, __LINE__,
				  __PRETTY_FUNCTION__);
	return true;
}

bool PutMethodHandler::canHandle(const std::string &method) const
{
	return method == "PUT";
}

bool PutMethodHandler::_ensureDirectory(const std::string &filePath) const
{
	size_t slashPos = filePath.find_last_of('/');
	if (slashPos == std::string::npos)
		return true;

	std::string dirPath = filePath.substr(0, slashPos);
	if (dirPath.empty())
		return true;

	std::vector<std::string> segments;
	size_t index = 0;
	while (index < dirPath.size())
	{
		while (index < dirPath.size() && dirPath[index] == '/')
			++index;
		size_t next = dirPath.find('/', index);
		if (next == std::string::npos)
			next = dirPath.size();
		if (next > index)
			segments.push_back(dirPath.substr(index, next - index));
		index = next;
	}

	std::string current = dirPath[0] == '/' ? std::string("/") : std::string();
	struct stat st;
	for (std::vector<std::string>::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		if (current.empty())
			current = *it;
		else if (current == "/")
			current += *it;
		else
			current += "/" + *it;

		if (stat(current.c_str(), &st) != 0)
		{
			if (mkdir(current.c_str(), 0755) != 0 && errno != EEXIST)
			{
				Logger::log(Logger::ERROR, "PutMethodHandler: mkdir failed for path: " + current +
											   " error: " + std::string(strerror(errno)));
				return false;
			}
		}
		else if (!S_ISDIR(st.st_mode))
		{
			Logger::log(Logger::ERROR, "PutMethodHandler: Path exists but is not a directory: " + current);
			return false;
		}
	}

	return true;
}

bool PutMethodHandler::_writeBodyToFile(const HttpRequest &request, const std::string &filePath) const
{
	std::ofstream output(filePath.c_str(), std::ios::binary | std::ios::trunc);
	if (!output.is_open())
	{
		Logger::log(Logger::ERROR, "PutMethodHandler: Unable to open file for writing: " + filePath);
		return false;
	}

	if (request.isUsingTempFile())
	{
		const std::string tempPath = request.getTempFile();
		std::ifstream input(tempPath.c_str(), std::ios::binary);
		if (!input.is_open())
		{
			Logger::log(Logger::ERROR, "PutMethodHandler: Unable to open temp file: " + tempPath);
			return false;
		}
		output << input.rdbuf();
	}
	else
	{
		const std::string body = request.getBodyData();
		if (!body.empty())
			output.write(body.c_str(), body.size());
	}

	output.flush();
	if (!output.good())
	{
		Logger::log(Logger::ERROR, "PutMethodHandler: Error occurred while writing file: " + filePath);
		return false;
	}

	return true;
}
