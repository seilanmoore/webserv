/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:27 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/11 18:24:13 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <vector>
#include <string>

#include "Request.hpp"
#include "Config.hpp"

// CGI state for non-blocking operation
struct CgiInfo
{
  pid_t pid;
  int stdinFd;
  int stdoutFd;
  std::vector<char> inputData;
  size_t inputWritten;
  std::string outputData;
  bool stdinClosed;
  bool active;

  CgiInfo() : pid(-1), stdinFd(-1), stdoutFd(-1), inputWritten(0), stdinClosed(false), active(false) {}
};

class Response
{
public:
  Response();
  Response(const Response &other);
  Response &operator=(const Response &other);
  ~Response();

  void generateResponse(const Request &request, const ServerConfig &serverConfig);

  ssize_t sendResponse(int fd);

  const std::string &getResponse() const;

  // Non-blocking CGI methods (public for Webserv access)
  void finalizeCgiResponse();
  void killCgi();

  // CGI state accessors
  bool hasPendingCgi() const { return _cgiInfo.active; }
  CgiInfo &getCgiInfo() { return _cgiInfo; }
  const CgiInfo &getCgiInfo() const { return _cgiInfo; }

private:
  // Status
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

  // Chunked transfer encoding
  bool _useChunked;
  bool _headersSent;
  size_t _chunkOffset;
  static const size_t CHUNK_SIZE = 8192;

  // HEAD request flag - body should not be sent
  bool _isHeadRequest;

  // CGI state for non-blocking operation
  CgiInfo _cgiInfo;

  // Private methods
  void handleGet(const Request &request, const ServerConfig &server,
                 const LocationConfig *location);
  void handlePost(const Request &request, const ServerConfig &server,
                  const LocationConfig *location);
  void handleDelete(const Request &request, const ServerConfig &server,
                    const LocationConfig *location);

  void serveFile(const std::string &filePath);
  void serveDirectory(const std::string &dirPath, const std::string &uri,
                      const LocationConfig *location, const ServerConfig &server);
  void generateDirectoryListing(const std::string &dirPath, const std::string &uri);
  void generateErrorPage(int code, const ServerConfig &server);
  void handleRedirect(int code, const std::string &url);
  void handleCgi(const Request &request, const std::string &scriptPath,
                 const std::string &interpreter);

  // Non-blocking CGI methods (private helpers)
  bool startCgi(const Request &request, const std::string &scriptPath,
                const std::string &interpreter);
  bool writeCgiInput();
  bool readCgiOutput();

  std::string getStatusMessage(int code) const;
  std::string getMimeType(const std::string &path) const;
  std::string buildFullPath(const std::string &root, const std::string &uri,
                            const std::string &locationPath) const;
  bool isDirectory(const std::string &path) const;
  bool fileExists(const std::string &path) const;
};

#endif
