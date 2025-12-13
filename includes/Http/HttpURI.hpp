#ifndef HTTPURI_HPP
#define HTTPURI_HPP

#include "../../includes/Config/Location.hpp"
#include "../../includes/Http/HttpResponse.hpp"
#include <cstddef>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// This class is responsible for parsing the URI from the request line
// It will also format the reponse if anything goes wrong
class HttpURI
{
public:
	// Parsing state
	enum URIState
	{
		URI_PARSING = 0,
		URI_PARSING_COMPLETE = 1,
		URI_PARSING_ERROR = 2
	};

private:
	// Parsing
	URIState _uriState;
	size_t _rawURISize;

	// Request line
	std::string _method;
	std::string _version;

	// Path Versions
	std::string _rawPath;
	std::string _decodedPath;
	std::string _resolvedPath;

	// Query parameters
	std::string _rawQueryString;
	std::string _decodedQueryString;
	std::map<std::string, std::vector<std::string> > _queryParameters;

	// Validation methods
	bool _validateMethod(const std::string &method, HttpResponse &response) const;
	bool _validatePath(const std::string &path, HttpResponse &response) const;
	bool _validateVersion(const std::string &version, HttpResponse &response) const;

	// Path resolution helper
	bool _resolvePathWithContainment(const std::string &path, const std::string &root, bool preserveTrailingSlash,
									 std::string &outResolved) const;

	// Query parameter parsing
	void _parseQueryParameters(const std::string &queryString);

public:
	// Constructor
	HttpURI();
	HttpURI(const HttpURI &other);
	~HttpURI();
	HttpURI &operator=(const HttpURI &other);

	// Main parsing method
	void parseBuffer(std::vector<char> &buffer, HttpResponse &response);

	// Sanitization and resolution
	void resolveURI(const Location *location, HttpResponse &response);

	// URI state accessor
	URIState getURIState() const;

	// Request line accessors
	const std::string &getMethod() const;
	const std::string &getRawPath() const;
	const std::string &getVersion() const;

	// URI versions accessors
	const std::string &getDecodedPath() const;
	const std::string &getResolvedPath() const;

	// Query parameters accessors
	const std::string &getRawQueryString() const;
	const std::string &getDecodedQueryString() const;
	const std::map<std::string, std::vector<std::string> > &getQueryParameters() const;

	// Utils
	void reset();
	size_t getRawURISize() const;
};

#endif /* ********************************************************* HTTPURI_H                                          \
		*/
