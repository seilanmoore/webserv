/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:27 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:54 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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

  // Non-blocking CGI methods (public for Webserv access)
  void finalizeCgiResponse();

  // Set raw response (for timeouts, errors)
  void setRawResponse(const std::string &response) { _response = response; }

  // CGI state accessors
  bool hasPendingCgi() const { return _cgiState.active; }
  CgiState &getCgiInfo() { return _cgiState; }
  const CgiState &getCgiInfo() const { return _cgiState; }

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

  // HEAD request flag - body should not be sent
  bool _isHeadRequest;

  // CGI state for non-blocking operation
  CgiState _cgiState;

  // Private methods
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

  // Non-blocking CGI methods (private helpers)
  bool startCgi(Request &request, const std::string &scriptPath,
                const std::string &interpreter);
};

#endif
