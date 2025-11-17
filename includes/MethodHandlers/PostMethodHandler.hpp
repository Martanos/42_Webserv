#ifndef POSTMETHODHANDLER_HPP
#define POSTMETHODHANDLER_HPP

#include "IMethodHandler.hpp"

class PostMethodHandler : public IMethodHandler {
public:
  PostMethodHandler();
  PostMethodHandler(const PostMethodHandler &other);
  ~PostMethodHandler();
  PostMethodHandler &operator=(const PostMethodHandler &other);

  // IMethodHandler implementation
  virtual bool handleRequest(const HttpRequest &request, HttpResponse &response,
                             const Location &location);

private:
  // Helper methods
  bool _validateUploadPath(const Location &location, HttpResponse &response);
  bool _generateUniqueFilename(const std::string &uploadPath,
                               const HttpRequest &request,
                               std::string &filePath);
  bool _saveUploadedFile(const std::string &filePath,
                         const HttpRequest &request);
};

#endif /* POSTMETHODHANDLER_HPP */
