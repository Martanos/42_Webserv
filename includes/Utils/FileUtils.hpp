#ifndef FILEUTILS_HPP
#define FILEUTILS_HPP

#include "../../includes/Utils/StrUtils.hpp"
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace FileUtils {
static inline bool isFileReadable(const std::string &filePath) {
  return access(filePath.c_str(), R_OK) == 0;
}

static inline bool isFileWritable(const std::string &filePath) {
  return access(filePath.c_str(), W_OK) == 0;
}

static inline bool isFileExecutable(const std::string &filePath) {
  return access(filePath.c_str(), X_OK) == 0;
}

static inline std::string normalizePath(const std::string &filePath) {
  char resolvedPath[4096];
  if (realpath(filePath.c_str(), resolvedPath) == NULL) {
    return "";
  }
  return std::string(resolvedPath);
}

static inline bool fileExists(const std::string &filePath) {
  return access(filePath.c_str(), F_OK) == 0;
}

static inline bool isDirectory(const std::string &path) {
  struct stat pathStat;
  if (stat(path.c_str(), &pathStat) != 0) {
    return false;
  }
  return S_ISDIR(pathStat.st_mode);
}

static inline bool isRegularFile(const std::string &path) {
  struct stat pathStat;
  if (stat(path.c_str(), &pathStat) != 0) {
    return false;
  }
  return S_ISREG(pathStat.st_mode);
}

static inline off_t getFileSize(const std::string &filePath) {
  struct stat fileStat;
  if (stat(filePath.c_str(), &fileStat) != 0) {
    return -1;
  }
  return fileStat.st_size;
}

static inline std::string generateFileName(const std::string &directory,
                                           const std::string &prefix,
                                           const std::string &extension) {
  std::string fileName;
  do {
    if (!prefix.empty())
      fileName = directory + "/" + prefix + "_" +
                 StrUtils::toString(time(NULL)) + "_" +
                 StrUtils::toString(rand()) + extension;
    else
      fileName = directory + "/" + StrUtils::toString(time(NULL)) + "_" +
                 StrUtils::toString(rand()) + extension;
  } while (fileExists(fileName));
  return fileName;
}
} // namespace FileUtils

#endif /* FILEUTILS_HPP */
