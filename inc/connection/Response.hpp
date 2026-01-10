/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:27 by smoore-a          #+#    #+#             */
/*   Updated: 2025/12/28 14:37:18 by smoore-a         ###   ########.fr       */
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

  std::string getStatusMessage(int code) const;
  std::string getMimeType(const std::string &path) const;
  std::string buildFullPath(const std::string &root, const std::string &uri,
                            const std::string &locationPath) const;
  bool isDirectory(const std::string &path) const;
  bool fileExists(const std::string &path) const;
};

#endif
