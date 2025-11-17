#include "../../includes/Global/MimeTypeResolver.hpp"
#include "../../includes/Global/Logger.hpp"
#include "../../includes/Utils/StrUtils.hpp"
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>

// Static extension to MIME type mapping
static std::map<std::string, std::string> *g_extensionMap = NULL;
static bool g_systemMimeLoaded = false;

/*
** ------------------------------- HELPER FUNCTIONS
* --------------------------------
*/

// Parse escape sequences in magic string (e.g., "\\x89" -> byte 0x89, "\\r" ->
// '\r', "\\n" -> '\n')
static std::string parseMagicString(const std::string &magicStr) {
  std::string result;
  result.reserve(magicStr.length());

  for (size_t i = 0; i < magicStr.length(); ++i) {
    if (magicStr[i] == '\\' && i + 1 < magicStr.length()) {
      switch (magicStr[i + 1]) {
      case 'x':
      case 'X':
        // Hex escape: \xHH
        if (i + 3 < magicStr.length()) {
          std::string hexStr = magicStr.substr(i + 2, 2);
          char *endPtr = NULL;
          long value = std::strtol(hexStr.c_str(), &endPtr, 16);
          if (endPtr && *endPtr == '\0' && value >= 0 && value <= 255) {
            result += static_cast<char>(value);
            i += 3; // Skip \xHH
            continue;
          }
        }
        // Invalid hex escape, treat as literal
        result += magicStr[i];
        break;
      case 'r':
        result += '\r';
        i += 1; // Skip \r
        continue;
      case 'n':
        result += '\n';
        i += 1; // Skip \n
        continue;
      case 't':
        result += '\t';
        i += 1; // Skip \t
        continue;
      case '\\':
        result += '\\';
        i += 1; // Skip backslash
        continue;
      default:
        // Unknown escape, treat as literal
        result += magicStr[i];
        break;
      }
    } else {
      result += magicStr[i];
    }
  }
  return result;
}

// Load MIME types from system file (/etc/mime.types)
// Format: mime/type<TAB>ext1 ext2 ext3...
static bool loadSystemMimeTypes(const std::string &filePath) {
  std::ifstream file(filePath.c_str());
  if (!file.is_open())
    return false;

  std::string line;
  size_t loadedCount = 0;

  while (std::getline(file, line)) {
    // Skip comments and empty lines
    if (line.empty() || line[0] == '#')
      continue;

    // Find tab separator (MIME type is before tab, extensions after)
    size_t tabPos = line.find('\t');
    if (tabPos == std::string::npos)
      continue;

    // Extract MIME type
    std::string mimeType = line.substr(0, tabPos);
    // Trim whitespace
    size_t mimeStart = mimeType.find_first_not_of(" \t");
    if (mimeStart == std::string::npos)
      continue;
    size_t mimeEnd = mimeType.find_last_not_of(" \t");
    mimeType = mimeType.substr(mimeStart, mimeEnd - mimeStart + 1);

    // Extract extensions (space-separated after tab)
    std::string extensions = line.substr(tabPos + 1);
    std::istringstream extStream(extensions);
    std::string ext;

    while (extStream >> ext) {
      ext = StrUtils::toLowerCase(ext);
      // Add extension (system file is loaded first, so map should be empty)
      (*g_extensionMap)[ext] = mimeType;
      ++loadedCount;
    }
  }

  file.close();
  Logger::debug("MimeTypeResolver: Loaded " + StrUtils::toString(loadedCount) +
                    " extensions from system file: " + filePath,
                __FILE__, __LINE__, __PRETTY_FUNCTION__);
  return true;
}

// Initialize custom fallback extension map
// Only adds extensions that aren't already present (system file takes
// precedence)
static void initializeCustomExtensionMap() {
  // Text types
  if (g_extensionMap->find("html") == g_extensionMap->end())
    (*g_extensionMap)["html"] = "text/html";
  if (g_extensionMap->find("htm") == g_extensionMap->end())
    (*g_extensionMap)["htm"] = "text/html";
  if (g_extensionMap->find("css") == g_extensionMap->end())
    (*g_extensionMap)["css"] = "text/css";
  if (g_extensionMap->find("txt") == g_extensionMap->end())
    (*g_extensionMap)["txt"] = "text/plain";
  if (g_extensionMap->find("xml") == g_extensionMap->end())
    (*g_extensionMap)["xml"] = "text/xml";
  if (g_extensionMap->find("csv") == g_extensionMap->end())
    (*g_extensionMap)["csv"] = "text/csv";

  // JavaScript
  if (g_extensionMap->find("js") == g_extensionMap->end())
    (*g_extensionMap)["js"] = "application/javascript";
  if (g_extensionMap->find("mjs") == g_extensionMap->end())
    (*g_extensionMap)["mjs"] = "application/javascript";

  // JSON
  if (g_extensionMap->find("json") == g_extensionMap->end())
    (*g_extensionMap)["json"] = "application/json";

  // Images
  if (g_extensionMap->find("png") == g_extensionMap->end())
    (*g_extensionMap)["png"] = "image/png";
  if (g_extensionMap->find("jpg") == g_extensionMap->end())
    (*g_extensionMap)["jpg"] = "image/jpeg";
  if (g_extensionMap->find("jpeg") == g_extensionMap->end())
    (*g_extensionMap)["jpeg"] = "image/jpeg";
  if (g_extensionMap->find("gif") == g_extensionMap->end())
    (*g_extensionMap)["gif"] = "image/gif";
  if (g_extensionMap->find("svg") == g_extensionMap->end())
    (*g_extensionMap)["svg"] = "image/svg+xml";
  if (g_extensionMap->find("webp") == g_extensionMap->end())
    (*g_extensionMap)["webp"] = "image/webp";
  if (g_extensionMap->find("ico") == g_extensionMap->end())
    (*g_extensionMap)["ico"] = "image/x-icon";
  if (g_extensionMap->find("bmp") == g_extensionMap->end())
    (*g_extensionMap)["bmp"] = "image/bmp";

  // Audio
  if (g_extensionMap->find("mp3") == g_extensionMap->end())
    (*g_extensionMap)["mp3"] = "audio/mpeg";
  if (g_extensionMap->find("wav") == g_extensionMap->end())
    (*g_extensionMap)["wav"] = "audio/wav";
  if (g_extensionMap->find("ogg") == g_extensionMap->end())
    (*g_extensionMap)["ogg"] = "audio/ogg";

  // Video
  if (g_extensionMap->find("mp4") == g_extensionMap->end())
    (*g_extensionMap)["mp4"] = "video/mp4";
  if (g_extensionMap->find("webm") == g_extensionMap->end())
    (*g_extensionMap)["webm"] = "video/webm";
  if (g_extensionMap->find("avi") == g_extensionMap->end())
    (*g_extensionMap)["avi"] = "video/x-msvideo";

  // Documents
  if (g_extensionMap->find("pdf") == g_extensionMap->end())
    (*g_extensionMap)["pdf"] = "application/pdf";
  if (g_extensionMap->find("doc") == g_extensionMap->end())
    (*g_extensionMap)["doc"] = "application/msword";
  if (g_extensionMap->find("docx") == g_extensionMap->end())
    (*g_extensionMap)["docx"] =
        "application/"
        "vnd.openxmlformats-officedocument.wordprocessingml.document";
  if (g_extensionMap->find("xls") == g_extensionMap->end())
    (*g_extensionMap)["xls"] = "application/vnd.ms-excel";
  if (g_extensionMap->find("xlsx") == g_extensionMap->end())
    (*g_extensionMap)["xlsx"] =
        "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
  if (g_extensionMap->find("ppt") == g_extensionMap->end())
    (*g_extensionMap)["ppt"] = "application/vnd.ms-powerpoint";
  if (g_extensionMap->find("pptx") == g_extensionMap->end())
    (*g_extensionMap)["pptx"] =
        "application/"
        "vnd.openxmlformats-officedocument.presentationml.presentation";

  // Archives
  if (g_extensionMap->find("zip") == g_extensionMap->end())
    (*g_extensionMap)["zip"] = "application/zip";
  if (g_extensionMap->find("tar") == g_extensionMap->end())
    (*g_extensionMap)["tar"] = "application/x-tar";
  if (g_extensionMap->find("gz") == g_extensionMap->end())
    (*g_extensionMap)["gz"] = "application/gzip";
  if (g_extensionMap->find("rar") == g_extensionMap->end())
    (*g_extensionMap)["rar"] = "application/x-rar-compressed";
  if (g_extensionMap->find("7z") == g_extensionMap->end())
    (*g_extensionMap)["7z"] = "application/x-7z-compressed";

  // Fonts
  if (g_extensionMap->find("ttf") == g_extensionMap->end())
    (*g_extensionMap)["ttf"] = "font/ttf";
  if (g_extensionMap->find("woff") == g_extensionMap->end())
    (*g_extensionMap)["woff"] = "font/woff";
  if (g_extensionMap->find("woff2") == g_extensionMap->end())
    (*g_extensionMap)["woff2"] = "font/woff2";
  if (g_extensionMap->find("eot") == g_extensionMap->end())
    (*g_extensionMap)["eot"] = "application/vnd.ms-fontobject";
  if (g_extensionMap->find("otf") == g_extensionMap->end())
    (*g_extensionMap)["otf"] = "font/otf";

  // Other
  if (g_extensionMap->find("bin") == g_extensionMap->end())
    (*g_extensionMap)["bin"] = "application/octet-stream";
  if (g_extensionMap->find("exe") == g_extensionMap->end())
    (*g_extensionMap)["exe"] = "application/x-msdownload";
  if (g_extensionMap->find("sh") == g_extensionMap->end())
    (*g_extensionMap)["sh"] = "application/x-sh";
}

// Initialize extension to MIME type mapping
// Tries system file first, then falls back to custom map
static void initializeExtensionMap() {
  if (g_extensionMap != NULL)
    return;

  g_extensionMap = new std::map<std::string, std::string>();
  // Try to load from system MIME types file first
  const char *systemMimePaths[] = {"/etc/mime.types", "/usr/share/mime/types",
                                   NULL};

  bool systemLoaded = false;
  for (int i = 0; systemMimePaths[i] != NULL; ++i) {
    if (loadSystemMimeTypes(systemMimePaths[i])) {
      systemLoaded = true;
      g_systemMimeLoaded = true;
      break;
    }
  }

  // Fall back to custom map for any missing extensions
  // (system file entries take precedence, so we add custom ones that aren't
  // already present)
  initializeCustomExtensionMap();

  if (!systemLoaded) {
    Logger::debug("MimeTypeResolver: System MIME types file not found, using "
                  "custom map only",
                  __FILE__, __LINE__, __PRETTY_FUNCTION__);
  }
}

/*
** ------------------------------- PUBLIC METHODS
* --------------------------------
*/

std::string MimeTypeResolver::resolveMimeType(const std::string &filePath) {
  // Try extension first (faster)
  std::string mimeType = resolveMimeTypeByExtension(filePath);
  if (mimeType != "application/octet-stream")
    return mimeType;

  // Fall back to magic bytes
  return resolveMimeTypeByMagic(filePath);
}

std::string
MimeTypeResolver::resolveMimeTypeByExtension(const std::string &filePath) {
  // Initialize extension map if needed
  if (g_extensionMap == NULL)
    initializeExtensionMap();

  // Extract extension
  size_t dotPos = filePath.find_last_of('.');
  if (dotPos == std::string::npos)
    return "application/octet-stream";

  std::string extension = filePath.substr(dotPos + 1);
  extension = StrUtils::toLowerCase(extension);

  // Look up in map
  std::map<std::string, std::string>::const_iterator it =
      g_extensionMap->find(extension);
  if (it != g_extensionMap->end())
    return it->second;

  return "application/octet-stream";
}

// Return the last extension part (after the final '.') in lowercase.
std::string MimeTypeResolver::getExtension(const std::string &filePath) {
  // Find last path separator to avoid dots in directories
  size_t lastSlash = filePath.find_last_of('/');
  if (lastSlash == std::string::npos)
    lastSlash = 0;
  else
    lastSlash += 1; // position of first char of filename

  size_t dotPos = filePath.find_last_of('.');
  if (dotPos == std::string::npos || dotPos < lastSlash)
    return std::string();

  // Hidden files like ".bashrc" should not be treated as having an extension
  if (dotPos == lastSlash)
    return std::string();

  std::string ext = filePath.substr(dotPos + 1);
  ext = StrUtils::toLowerCase(ext);
  return ext;
}

// Return all individual extension parts in right-to-left order. E.g.
// "archive.tar.gz" -> {"gz","tar"}
std::vector<std::string>
MimeTypeResolver::getExtensions(const std::string &filePath) {
  std::vector<std::string> result;

  // Extract filename (after last slash)
  size_t lastSlash = filePath.find_last_of('/');
  size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
  std::string filename = filePath.substr(start);

  // If filename starts with '.' and contains no other dots, it's a hidden file
  // with no extension
  if (!filename.empty() && filename[0] == '.') {
    size_t otherDot = filename.find('.', 1);
    if (otherDot == std::string::npos)
      return result;
  }

  // Split by '.'
  std::vector<std::string> parts;
  size_t pos = 0;
  while (true) {
    size_t dot = filename.find('.', pos);
    if (dot == std::string::npos) {
      parts.push_back(filename.substr(pos));
      break;
    }
    parts.push_back(filename.substr(pos, dot - pos));
    pos = dot + 1;
  }

  if (parts.size() <= 1)
    return result;

  // Collect extension parts from right to left, skip the base name (parts[0])
  for (size_t i = parts.size() - 1; i >= 1; --i) {
    std::string p = StrUtils::toLowerCase(parts[i]);
    result.push_back(p);
    if (i == 1)
      break; // avoid size_t underflow
  }

  return result;
}

// Return the compound extension (everything after the first dot in the
// filename) in lowercase. E.g. "archive.tar.gz" -> "tar.gz"
std::string
MimeTypeResolver::getCompoundExtension(const std::string &filePath) {
  // Extract filename
  size_t lastSlash = filePath.find_last_of('/');
  size_t start = (lastSlash == std::string::npos) ? 0 : lastSlash + 1;
  std::string filename = filePath.substr(start);

  size_t firstDot = filename.find('.');
  if (firstDot == std::string::npos)
    return std::string();

  // Hidden file with no extension
  if (firstDot == 0) {
    size_t other = filename.find('.', 1);
    if (other == std::string::npos)
      return std::string();
    firstDot = other; // compound starts after second dot
  }

  std::string compound = filename.substr(firstDot + 1);
  return StrUtils::toLowerCase(compound);
}

// Return a common extension for a given MIME type or header string.
std::string
MimeTypeResolver::getExtensionByMimeType(const std::string &headerOrMime) {
  if (g_extensionMap == NULL)
    initializeExtensionMap();

  // Extract mime type portion if full header was provided (e.g., "Content-Type:
  // text/html; charset=utf-8")
  std::string mime = headerOrMime;
  // Find ':' and take substring after it if present
  size_t colon = mime.find(':');
  if (colon != std::string::npos)
    mime = mime.substr(colon + 1);

  // Drop parameters after ';'
  size_t semi = mime.find(';');
  if (semi != std::string::npos)
    mime = mime.substr(0, semi);

  // Trim whitespace
  size_t start = mime.find_first_not_of(" \t");
  if (start == std::string::npos)
    return std::string();
  size_t end = mime.find_last_not_of(" \t");
  mime = mime.substr(start, end - start + 1);

  mime = StrUtils::toLowerCase(mime);

  // Search map for first extension that matches this mime type
  for (std::map<std::string, std::string>::const_iterator it =
           g_extensionMap->begin();
       it != g_extensionMap->end(); ++it) {
    if (StrUtils::toLowerCase(it->second) == mime)
      return it->first;
  }

  return std::string();
}

// Detect MIME type by magic and return a matching extension (first found), or
// empty string on failure.
std::string MimeTypeResolver::getExtensionByMagic(const std::string &filePath) {
  std::string mime = resolveMimeTypeByMagic(filePath);
  if (mime == "application/octet-stream")
    return std::string();

  return getExtensionByMimeType(mime);
}

std::string
MimeTypeResolver::resolveMimeTypeByMagic(const std::string &filePath) {
  // Open file in binary mode
  std::ifstream file(filePath.c_str(), std::ios::binary);
  if (!file.is_open())
    return "application/octet-stream";

  // Read first 512 bytes (enough for most magic numbers)
  const size_t maxBytes = 512;
  char buffer[maxBytes];
  file.read(buffer, maxBytes);
  size_t bytesRead = static_cast<size_t>(file.gcount());
  file.close();

  if (bytesRead == 0)
    return "application/octet-stream";

  // Get magic rules
  std::vector<MagicRule> &rules = getMagicRules();

  // Try each rule
  for (size_t i = 0; i < rules.size(); ++i) {
    const MagicRule &rule = rules[i];

    // Parse the magic string to get actual bytes
    std::string parsedMagic = parseMagicString(rule.magic);

    // Check if we have enough bytes
    if (rule.offset + parsedMagic.length() > bytesRead)
      continue;

    // Check if magic bytes match
    bool match = true;
    for (size_t j = 0; j < parsedMagic.length(); ++j) {
      if (static_cast<size_t>(rule.offset + j) >= bytesRead ||
          buffer[rule.offset + j] != parsedMagic[j]) {
        match = false;
        break;
      }
    }

    if (match)
      return rule.mimeType;
  }

  return "application/octet-stream";
}

void MimeTypeResolver::initialize() {
  // Initialize extension map
  initializeExtensionMap();

  // Mark as initialized
  getInitialized() = true;

  Logger::debug("MimeTypeResolver: Initialized with " +
                    StrUtils::toString(g_extensionMap->size()) +
                    " extension mappings",
                __FILE__, __LINE__, __PRETTY_FUNCTION__);
}

void MimeTypeResolver::cleanup() {
  if (g_extensionMap != NULL) {
    delete g_extensionMap;
    g_extensionMap = NULL;
  }

  getInitialized() = false;
  Logger::debug("MimeTypeResolver: Cleaned up", __FILE__, __LINE__,
                __PRETTY_FUNCTION__);
}
