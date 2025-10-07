#include "../../includes/DeleteMethodHandler.hpp"
#include "../../includes/PerformanceMonitor.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

DeleteMethodHandler::DeleteMethodHandler()
{
}

DeleteMethodHandler::DeleteMethodHandler(const DeleteMethodHandler &src)
{
	(void)src;
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

DeleteMethodHandler::~DeleteMethodHandler()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

DeleteMethodHandler &DeleteMethodHandler::operator=(DeleteMethodHandler const &rhs)
{
	(void)rhs;
	return *this;
}

/*
** --------------------------------- METHODS ----------------------------------
*/


bool DeleteMethodHandler::isDeletionAllowed(const std::string &filePath, const Location *location) const
{
	// Only allow deletion in upload directories
	if (!location || location->getUploadPath().empty())
	{
		return false;
	}

	// Check if the file is within the upload path
	std::string uploadPath = location->getUploadPath();

	// Resolve real paths to prevent directory traversal attacks
	char resolvedFile[PATH_MAX];
	char resolvedUpload[PATH_MAX];

	if (realpath(filePath.c_str(), resolvedFile) == NULL)
	{
		return false;
	}

	if (realpath(uploadPath.c_str(), resolvedUpload) == NULL)
	{
		return false;
	}

	// Check if file is within upload directory
	std::string fileStr(resolvedFile);
	std::string uploadStr(resolvedUpload);

	return fileStr.find(uploadStr) == 0;
}

bool DeleteMethodHandler::deleteFile(const std::string &filePath) const
{
	if (unlink(filePath.c_str()) == 0)
	{
		Logger::log(Logger::INFO, "File deleted: " + filePath);
		return true;
	}
	else
	{
		Logger::log(Logger::ERROR, "Failed to delete file: " + filePath);
		return false;
	}
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

bool DeleteMethodHandler::canHandle(const std::string &method) const
{
	return method == "DELETE";
}

std::string DeleteMethodHandler::getMethod() const
{
	return "DELETE";
}

/* ************************************************************************** */
