#include "../../includes/RequestRouter.hpp"

/*
** ------------------------------- CONSTRUCTOR --------------------------------
*/

RequestRouter::RequestRouter()
{
}

/*
** -------------------------------- DESTRUCTOR --------------------------------
*/

RequestRouter::~RequestRouter()
{
}

/*
** --------------------------------- OVERLOAD ---------------------------------
*/

/*
** --------------------------------- METHODS ----------------------------------
*/

void RequestRouter::route(const HttpRequest &request, HttpResponse &response, const Server *server)
{
            response.setBody(server->getStatusPage(405));    if (server == NULL)
    {
        Logger::error("RequestRouter: No server configuration provided to router");
        generateErrorResponse(response, 500, NULL);
        return;
    }

    try
    {
        std::string method = request.getMethod();
        std::string uri = request.getUri();

        Logger::info("Routing " + method + " request for " + uri);

        // --- Match Location (safe reference) ---
        const Location *location = matchLocation(uri, server);
        if (location)
            Logger::debug("Matched location: " + location->getPath());
        else
            Logger::debug("No matching location, using default server root");

        // --- Verify method allowed ---
        if (!isMethodAllowed(method, location, server))
        {
            Logger::debug("RequestRouter: Method " + method + " not allowed");
            response.setStatus(405, "Method Not Allowed");

            response.setHeader("Content-Type", "text/html");
            response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
            response.setHeader("Allow", getAllowedMethodsString(location, server));
            return;
        }

        // --- Get method handler ---
        MethodHandlerFactory &factory = MethodHandlerFactory::getInstance();
        IMethodHandler *handler = factory.getHandler(method);
        if (handler == NULL)
        {
            Logger::error("RequestRouter: No handler found for " + method);
            response.setStatus(501, "Not Implemented");
            response.setBody(server->getStatusPage(501));
            response.setHeader("Content-Type", "text/html");
            response.setHeader("Content-Length", StringUtils::toString(response.getBody().length()));
            return;
        }

        // --- Dispatch safely ---
        handler->handle(request, response, server, location);

        // --- Postprocess common headers ---
        response.setHeader("Server", "42_Webserv/1.0");

        // Date header
        time_t now = time(NULL);
        struct tm *tm = gmtime(&now);
        char dateBuf[128];
        if (tm)
        {
            strftime(dateBuf, sizeof(dateBuf), "%a, %d %b %Y %H:%M:%S GMT", tm);
            response.setHeader("Date", std::string(dateBuf));
        }

        // Connection
        std::string connection;
        if (!request.getHeader("connection").empty())
            connection = request.getHeader("connection")[0];

        if (connection == "close" || request.getVersion() == "HTTP/1.0")
            response.setHeader("Connection", "close");
        else
            response.setHeader("Connection", "keep-alive");
    }
    catch (const std::exception &e)
    {
        Logger::error(std::string("RequestRouter: Exception: ") + e.what());
        generateErrorResponse(response, 500, server);
    }
    catch (...)
    {
        Logger::error("RequestRouter: Unknown exception caught");
        generateErrorResponse(response, 500, server);
    }
}

const Location *RequestRouter::matchLocation(const std::string &uri, const Server *server) const
{
    if (server == NULL)
        return NULL;

    const std::map<std::string, Location> &locations = server->getLocations();
    const Location *bestMatch = NULL;
    size_t bestLen = 0;

    for (std::map<std::string, Location>::const_iterator it = locations.begin();
         it != locations.end(); ++it)
    {
        const std::string &path = it->first;
        if (uri.find(path) == 0 && path.length() > bestLen)
        {
            bestMatch = &(it->second);
            bestLen = path.length();
        }
    }
    return bestMatch;
}

/*
** ----------------------------- METHOD ALLOWED -------------------------------
*/

bool RequestRouter::isMethodAllowed(const std::string &method,
                                    const Location *location,
                                    const Server *server) const
{
    (void)server;

    if (location == NULL)
        return (method == "GET" || method == "HEAD");

    const std::vector<std::string> &allowed = location->getAllowedMethods();
    if (allowed.empty())
        return (method == "GET" || method == "HEAD");

    for (std::vector<std::string>::const_iterator it = allowed.begin(); it != allowed.end(); ++it)
    {
        if (*it == method)
            return true;
        if (*it == "GET" && method == "HEAD")
            return true;
    }
    return false;
}

/*
** ------------------------------ ALLOW HEADER --------------------------------
*/

std::string RequestRouter::getAllowedMethodsString(const Location *location,
                                                   const Server *server) const
{
    (void)server;
    std::string allowed;
    if (location == NULL || location->getAllowedMethods().empty())
        return "GET, HEAD";

    const std::vector<std::string> &methods = location->getAllowedMethods();
    for (size_t i = 0; i < methods.size(); ++i)
    {
        if (i > 0)
            allowed += ", ";
        allowed += methods[i];
        if (methods[i] == "GET")
            allowed += ", HEAD";
    }
    return allowed;
}

/*
** --------------------------- ERROR RESPONSE GEN -----------------------------
*/

void RequestRouter::generateErrorResponse(HttpResponse &response,
                                          int code,
                                          const Server *server) const
{
    response.setStatus(code, DefaultStatusMap::getStatusMessage(code));
    std::string body;

    if (server)
        body = server->getStatusPage(code);
    if (body.empty())
        body = DefaultStatusMap::getStatusBody(code);

    response.setBody(body);
    response.setHeader("Content-Type", "text/html");
    response.setHeader("Content-Length", StringUtils::toString(body.length()));
    response.setHeader("Server", "42_Webserv/1.0");
}

/*
** --------------------------------- ACCESSOR ---------------------------------
*/

/* ************************************************************************** */
