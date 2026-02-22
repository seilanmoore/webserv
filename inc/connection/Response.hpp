#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <vector>
#include <string>

#include "Types.hpp"
#include "Request.hpp"
#include "Config.hpp"

class Response
{
public:
  Response();
  Response(const Response &other);
  Response &operator=(const Response &other);
  ~Response();

  void generateResponse(Request &request, const ServerConfig &serverConfig);

  ssize_t sendResponse(int fd);

  void finalizeCgiResponse();

  void setRawResponse(const std::string &response);

  bool hasPendingCgi() const;
  CgiState &getCgiInfo();
  const CgiState &getCgiInfo() const;

private:
  int _statusCode;
  std::string _statusMessage;

  bool _sendingBinary;
  ssize_t _binaryBytesSent;
  ssize_t _bytesSent;

  std::string _contentType;
  std::string _filename;
  bool _fileFound;
  std::string _fileContent;

  bool _isBinary;
  std::vector<char> _binaryContent;

  ssize_t _contentLength;
  std::string _response;

  bool _useChunked;
  bool _headersSent;
  size_t _chunkOffset;

  bool _isHeadRequest;

  CgiState _cgiState;

  void handleGet(Request &request, const ServerConfig &server,
                 const LocationConfig *location);
  void handlePost(Request &request, const ServerConfig &server,
                  const LocationConfig *location);
  void handleDelete(const Request &request, const ServerConfig &server,
                    const LocationConfig *location);

  void serveFile(const std::string &filePath);
  void serveDirectory(const std::string &dirPath, const std::string &uri,
                      const LocationConfig *location, const ServerConfig &server);
  void generateDirectoryListing(const std::string &dirPath, const std::string &uri);
  void generateErrorPage(int code, const ServerConfig &server);
  void handleRedirect(int code, const std::string &url);
  void handleCgi(Request &request, const std::string &scriptPath,
                 const std::string &interpreter);

  bool startCgi(Request &request, const std::string &scriptPath,
                const std::string &interpreter);
};

#endif
