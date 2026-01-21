/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:31 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:54 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"
#include "HttpUtils.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <ctime>

#include "Request.hpp"
#include "Config.hpp"
#include "utils.hpp"

// ============================================================================
// Constructor / Destructor
// ============================================================================

Response::Response()
    : _statusCode(200),
      _statusMessage("OK"),
      _sendingBinary(false),
      _binaryBytesSent(0),
      _bytesSent(0),
      _contentType("text/html"),
      _filename(),
      _fileFound(false),
      _fileContent(),
      _isBinary(false),
      _binaryContent(),
      _contentLength(0),
      _response(),
      _useChunked(false),
      _headersSent(false),
      _chunkOffset(0),
      _isHeadRequest(false)
{
}

Response::Response(const Response &other)
{
  (void)other;
}

Response &Response::operator=(const Response &other)
{
  if (this == &other)
    return *this;
  return *this;
}

Response::~Response()
{
}

// ============================================================================
// Main Response Generation
// ============================================================================

void Response::generateResponse(Request &request, const ServerConfig &server)
{
  const std::string &method = request.getMethod();
  const std::string &uri = request.getPath();

  // Track HEAD requests - they use GET logic but must not send body
  _isHeadRequest = (method == "HEAD");

  // Find matching location
  const LocationConfig *location = server.matchLocation(uri);

  // Handle redirections first (before method check)
  if (location && location->redirectCode != 0)
  {
    handleRedirect(location->redirectCode, location->redirectUrl);
    return;
  }

  // Check if method is allowed
  if (location)
  {
    if (location->allowedMethods.find(method) == location->allowedMethods.end())
    {
      generateErrorPage(405, server);
      return;
    }
  }

  // Check body size limit (location setting takes precedence over server setting)
  size_t maxBodySize = server.clientMaxBodySize;
  if (location && location->clientMaxBodySize > 0)
    maxBodySize = location->clientMaxBodySize;

  if (request.getContentLength() > maxBodySize)
  {
    generateErrorPage(413, server);
    return;
  }

  // Route to appropriate handler
  if (method == "GET" || method == "HEAD")
    handleGet(request, server, location);
  else if (method == "POST")
    handlePost(request, server, location);
  else if (method == "DELETE")
    handleDelete(request, server, location);
  else
    generateErrorPage(501, server);
}

// ============================================================================
// HTTP Method Handlers
// ============================================================================

void Response::handleGet(Request &request, const ServerConfig &server,
                         const LocationConfig *location)
{
  std::string uri = request.getPath();
  std::string root = location ? location->root : server.root;
  std::string locationPath = location ? location->path : "/";

  // Build full file path
  std::string fullPath = HttpUtils::buildFullPath(root, uri, locationPath);

  // Check if path exists
  if (!HttpUtils::fileExists(fullPath))
  {
    generateErrorPage(404, server);
    return;
  }

  // Handle directory
  if (HttpUtils::isDirectory(fullPath))
  {
    serveDirectory(fullPath, uri, location, server);
    return;
  }

  // Check for CGI
  if (location && location->cgiEnable)
  {
    std::string ext = request.getFileExtension();
    std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(ext);
    if (it != location->cgiPass.end())
    {
      handleCgi(request, fullPath, it->second);
      return;
    }
  }

  // Serve file
  serveFile(fullPath);
}

void Response::handlePost(Request &request, const ServerConfig &server,
                          const LocationConfig *location)
{
  std::string uri = request.getPath();
  std::string root = location ? location->root : server.root;
  std::string locationPath = location ? location->path : "/";

  // Check for CGI
  if (location && location->cgiEnable)
  {
    std::string ext = request.getFileExtension();
    std::map<std::string, std::string>::const_iterator it = location->cgiPass.find(ext);
    if (it != location->cgiPass.end())
    {
      std::string fullPath = HttpUtils::buildFullPath(root, uri, locationPath);
      handleCgi(request, fullPath, it->second);
      return;
    }
  }

  // Handle file upload
  if (location && location->uploadEnable)
  {
    const std::vector<char> &body = request.getBody();
    if (body.empty())
    {
      generateErrorPage(400, server);
      return;
    }

    // Determine upload path
    std::string uploadDir = location->uploadStore;
    if (uploadDir.empty())
      uploadDir = "./uploads";

    // Create simple filename from URI or use timestamp
    std::string filename;
    size_t lastSlash = uri.find_last_of('/');
    if (lastSlash != std::string::npos && lastSlash < uri.length() - 1)
      filename = uri.substr(lastSlash + 1);
    else
    {
      std::ostringstream oss;
      oss << "upload_" << time(NULL);
      filename = oss.str();
    }

    std::string uploadPath = uploadDir + "/" + filename;

    // Write file
    std::ofstream outFile(uploadPath.c_str(), std::ios::binary);
    if (!outFile.is_open())
    {
      generateErrorPage(500, server);
      return;
    }

    outFile.write(&body[0], body.size());
    outFile.close();

    // Success response
    _statusCode = 201;
    _statusMessage = "Created";
    _contentType = "text/html";
    _fileContent = "<html><body><h1>File uploaded successfully</h1>"
                   "<p>Saved to: " +
                   uploadPath + "</p></body></html>";
    _fileFound = true;
  }
  else
  {
    // No CGI, no upload - just accept the POST and return 200
    _statusCode = 200;
    _statusMessage = "OK";
    _contentType = "text/html";
    _fileContent = "<html><body><h1>POST received</h1></body></html>";
    _fileFound = true;
  }

  // Build response
  std::ostringstream oss;
  _contentLength = _fileContent.length();
  oss << _contentLength;

  std::ostringstream codeStr;
  codeStr << _statusCode;

  _response = "HTTP/1.1 " + codeStr.str() + " " + _statusMessage + "\r\n"
                                                                   "Content-Type: " +
              _contentType + "\r\n"
                             "Content-Length: " +
              oss.str() + "\r\n"
                          "\r\n" +
              _fileContent;
}

void Response::handleDelete(const Request &request, const ServerConfig &server,
                            const LocationConfig *location)
{
  std::string uri = request.getPath();
  std::string locationPath = location ? location->path : "/";

  // For upload locations, use upload_store; otherwise use root
  std::string root;
  if (location && location->uploadEnable && !location->uploadStore.empty())
    root = location->uploadStore;
  else
    root = location ? location->root : server.root;

  std::string fullPath = HttpUtils::buildFullPath(root, uri, locationPath);

  // Check if file exists
  if (!HttpUtils::fileExists(fullPath))
  {
    generateErrorPage(404, server);
    return;
  }

  // Don't allow deleting directories
  if (HttpUtils::isDirectory(fullPath))
  {
    generateErrorPage(403, server);
    return;
  }

  // Try to delete
  if (std::remove(fullPath.c_str()) != 0)
  {
    generateErrorPage(500, server);
    return;
  }

  // Success
  _statusCode = 200;
  _statusMessage = "OK";
  _contentType = "text/html";
  _fileContent = "<html><body><h1>File deleted successfully</h1></body></html>";
  _fileFound = true;

  std::ostringstream oss;
  _contentLength = _fileContent.length();
  oss << _contentLength;

  _response = "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/html\r\n"
              "Content-Length: " +
              oss.str() + "\r\n"
                          "\r\n" +
              _fileContent;
}

// ============================================================================
// File Serving
// ============================================================================

void Response::serveFile(const std::string &filePath)
{
  _contentType = HttpUtils::getMimeType(filePath);
  _filename = filePath;

  // Check if binary
  _isBinary = (_contentType.find("text/") == std::string::npos &&
               _contentType.find("application/json") == std::string::npos &&
               _contentType.find("application/javascript") == std::string::npos &&
               _contentType.find("application/xml") == std::string::npos);

  if (_isBinary)
  {
    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (file.is_open())
    {
      file.seekg(0, std::ios::end);
      std::streamsize size = file.tellg();
      file.seekg(0, std::ios::beg);
      _binaryContent.resize(size);
      if (size > 0)
        file.read(&_binaryContent[0], size);
      file.close();
      _fileFound = true;
      _contentLength = size;

      // Use chunked for large files
      if (static_cast<size_t>(size) > CHUNKED_THRESHOLD)
        _useChunked = true;
    }
  }
  else
  {
    std::ifstream file(filePath.c_str());
    if (file.is_open())
    {
      std::stringstream buffer;
      buffer << file.rdbuf();
      _fileContent = buffer.str();
      file.close();
      _fileFound = true;
      _contentLength = _fileContent.length();

      // Use chunked for large files
      if (_fileContent.length() > CHUNKED_THRESHOLD)
        _useChunked = true;
    }
  }

  if (!_fileFound)
  {
    _statusCode = 500;
    _statusMessage = "Internal Server Error";
    _fileContent = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    _contentType = "text/html";
    _contentLength = _fileContent.length();
  }
  else
  {
    _statusCode = 200;
    _statusMessage = "OK";
  }

  // Build response headers
  std::ostringstream codeStr;
  codeStr << _statusCode;

  if (_useChunked)
  {
    // Chunked transfer encoding - no Content-Length
    _response = "HTTP/1.1 " + codeStr.str() + " " + _statusMessage + "\r\n"
                                                                     "Content-Type: " +
                _contentType + "\r\n"
                               "Transfer-Encoding: chunked\r\n"
                               "\r\n";
  }
  else
  {
    std::ostringstream oss;
    oss << _contentLength;

    _response = "HTTP/1.1 " + codeStr.str() + " " + _statusMessage + "\r\n"
                                                                     "Content-Type: " +
                _contentType + "\r\n"
                               "Content-Length: " +
                oss.str() + "\r\n"
                            "\r\n";

    // For HEAD requests, don't include body
    if (!_isBinary && !_isHeadRequest)
      _response += _fileContent;
  }
}

void Response::serveDirectory(const std::string &dirPath, const std::string &uri,
                              const LocationConfig *location, const ServerConfig &server)
{
  // Try index files
  std::vector<std::string> indexFiles;
  if (location && !location->index.empty())
    indexFiles = location->index;
  else
    indexFiles = server.index;

  for (size_t i = 0; i < indexFiles.size(); ++i)
  {
    std::string indexPath = dirPath;
    if (indexPath[indexPath.length() - 1] != '/')
      indexPath += "/";
    indexPath += indexFiles[i];

    if (HttpUtils::fileExists(indexPath) && !HttpUtils::isDirectory(indexPath))
    {
      serveFile(indexPath);
      return;
    }
  }

  // Check autoindex
  bool autoindex = location ? location->autoindex : false;
  if (autoindex)
  {
    generateDirectoryListing(dirPath, uri);
    return;
  }

  // No index file and no autoindex - return 404
  generateErrorPage(404, server);
}

void Response::generateDirectoryListing(const std::string &dirPath, const std::string &uri)
{
  DIR *dir = opendir(dirPath.c_str());
  if (!dir)
  {
    _statusCode = 500;
    _fileContent = "<html><body><h1>500 Internal Server Error</h1></body></html>";
    _contentType = "text/html";
    _contentLength = _fileContent.length();

    std::ostringstream oss;
    oss << _contentLength;
    _response = "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " +
                oss.str() + "\r\n\r\n" + _fileContent;
    return;
  }

  std::ostringstream html;
  html << "<!DOCTYPE html>\n<html>\n<head>\n"
       << "<title>Index of " << uri << "</title>\n"
       << "<style>body{font-family:monospace;} a{text-decoration:none;} "
       << "table{border-collapse:collapse;} td{padding:5px 20px;}</style>\n"
       << "</head>\n<body>\n"
       << "<h1>Index of " << uri << "</h1>\n"
       << "<hr>\n<table>\n";

  // Parent directory link
  if (uri != "/" && !uri.empty())
  {
    std::string parent = uri;
    if (parent[parent.length() - 1] == '/')
      parent.erase(parent.length() - 1);
    size_t lastSlash = parent.find_last_of('/');
    if (lastSlash != std::string::npos)
      parent = parent.substr(0, lastSlash + 1);
    else
      parent = "/";
    html << "<tr><td><a href=\"" << parent << "\">../</a></td><td>-</td></tr>\n";
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL)
  {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;

    std::string entryPath = dirPath;
    if (entryPath[entryPath.length() - 1] != '/')
      entryPath += "/";
    entryPath += name;

    struct stat st;
    if (stat(entryPath.c_str(), &st) == 0)
    {
      std::string linkUri = uri;
      if (linkUri[linkUri.length() - 1] != '/')
        linkUri += "/";
      linkUri += name;

      if (S_ISDIR(st.st_mode))
      {
        html << "<tr><td><a href=\"" << linkUri << "/\">" << name << "/</a></td>"
             << "<td>-</td></tr>\n";
      }
      else
      {
        html << "<tr><td><a href=\"" << linkUri << "\">" << name << "</a></td>"
             << "<td>" << st.st_size << " bytes</td></tr>\n";
      }
    }
  }
  closedir(dir);

  html << "</table>\n<hr>\n</body>\n</html>";

  _statusCode = 200;
  _statusMessage = "OK";
  _contentType = "text/html";
  _fileContent = html.str();
  _fileFound = true;
  _contentLength = _fileContent.length();

  std::ostringstream oss;
  oss << _contentLength;

  _response = "HTTP/1.1 200 OK\r\n"
              "Content-Type: text/html\r\n"
              "Content-Length: " +
              oss.str() + "\r\n"
                          "\r\n";

  // For HEAD requests, don't include body
  if (!_isHeadRequest)
    _response += _fileContent;
}

// ============================================================================
// Error Pages
// ============================================================================

void Response::generateErrorPage(int code, const ServerConfig &server)
{
  _statusCode = code;
  _statusMessage = HttpUtils::getStatusMessage(code);
  _contentType = "text/html";

  // Check for custom error page
  std::map<int, std::string>::const_iterator it = server.errorPages.find(code);
  if (it != server.errorPages.end())
  {
    std::string errorPath = server.root + it->second;
    std::ifstream file(errorPath.c_str());
    if (file.is_open())
    {
      std::stringstream buffer;
      buffer << file.rdbuf();
      _fileContent = buffer.str();
      file.close();
    }
    else
    {
      // Fallback to default
      std::ostringstream html;
      html << "<html><head><title>" << code << " " << _statusMessage << "</title></head>"
           << "<body><h1>" << code << " " << _statusMessage << "</h1></body></html>";
      _fileContent = html.str();
    }
  }
  else
  {
    // Default error page
    std::ostringstream html;
    html << "<!DOCTYPE html>\n<html>\n<head>\n"
         << "<title>" << code << " " << _statusMessage << "</title>\n"
         << "<style>body{font-family:Arial,sans-serif;text-align:center;padding-top:50px;}</style>\n"
         << "</head>\n<body>\n"
         << "<h1>" << code << " " << _statusMessage << "</h1>\n"
         << "<hr>\n<p>webserv</p>\n"
         << "</body>\n</html>";
    _fileContent = html.str();
  }

  _fileFound = true;
  _contentLength = _fileContent.length();

  std::ostringstream lengthStr;
  lengthStr << _contentLength;

  std::ostringstream codeStr;
  codeStr << code;

  _response = "HTTP/1.1 " + codeStr.str() + " " + _statusMessage + "\r\n"
                                                                   "Content-Type: text/html\r\n"
                                                                   "Content-Length: " +
              lengthStr.str() + "\r\n"
                                "\r\n";

  // For HEAD requests, don't include body
  if (!_isHeadRequest)
    _response += _fileContent;
}

// ============================================================================
// Redirections
// ============================================================================

void Response::handleRedirect(int code, const std::string &url)
{
  _statusCode = code;
  _statusMessage = HttpUtils::getStatusMessage(code);
  _contentType = "text/html";

  std::ostringstream html;
  html << "<!DOCTYPE html>\n<html>\n<head>\n"
       << "<title>" << code << " " << _statusMessage << "</title>\n"
       << "</head>\n<body>\n"
       << "<h1>" << code << " " << _statusMessage << "</h1>\n"
       << "<p>Redirecting to <a href=\"" << url << "\">" << url << "</a></p>\n"
       << "</body>\n</html>";

  _fileContent = html.str();
  _fileFound = true;
  _contentLength = _fileContent.length();

  std::ostringstream lengthStr;
  lengthStr << _contentLength;

  std::ostringstream codeStr;
  codeStr << code;

  _response = "HTTP/1.1 " + codeStr.str() + " " + _statusMessage + "\r\n"
                                                                   "Location: " +
              url + "\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " +
              lengthStr.str() + "\r\n"
                                "\r\n";

  // For HEAD requests, don't include body
  if (!_isHeadRequest)
    _response += _fileContent;
}

// ============================================================================
// Send Response
// ============================================================================

ssize_t Response::sendResponse(int fd)
{
  ssize_t bytesSent;

  // Handle chunked transfer encoding
  if (_useChunked)
  {
    // First send headers if not sent
    if (!_headersSent)
    {
      bytesSent = send(fd, _response.c_str(), _response.length(), 0);
      if (bytesSent == -1)
        return -1;
      _headersSent = true;
      return bytesSent;
    }

    // Determine the content source (binary or text)
    const char *contentData;
    size_t contentSize;
    if (_isBinary && !_binaryContent.empty())
    {
      contentData = &_binaryContent[0];
      contentSize = _binaryContent.size();
    }
    else
    {
      contentData = _fileContent.c_str();
      contentSize = _fileContent.length();
    }

    // Check if we're done
    if (_chunkOffset >= contentSize)
    {
      // Send final chunk (0\r\n\r\n)
      const char *finalChunk = "0\r\n\r\n";
      bytesSent = send(fd, finalChunk, 5, 0);
      if (bytesSent == -1)
        return -1;
      return 0; // Done
    }

    // Calculate chunk size
    size_t remaining = contentSize - _chunkOffset;
    size_t chunkSize = remaining > CHUNK_SIZE ? CHUNK_SIZE : remaining;

    // Build complete chunk: <size in hex>\r\n<data>\r\n
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << chunkSize << "\r\n";
    std::string chunk = chunkHeader.str();
    chunk.append(contentData + _chunkOffset, chunkSize);
    chunk.append("\r\n");

    // Send entire chunk with a single write
    bytesSent = send(fd, chunk.c_str(), chunk.length(), 0);
    if (bytesSent == -1)
      return -1;

    _chunkOffset += chunkSize;
    return static_cast<ssize_t>(_chunkOffset);
  }

  // Regular (non-chunked) response handling
  if (_sendingBinary)
  {
    if (_contentLength - _binaryBytesSent >= static_cast<ssize_t>(BUFFER_SIZE))
      bytesSent = send(fd, &_binaryContent[_binaryBytesSent], BUFFER_SIZE, 0);
    else
      bytesSent = send(fd, &_binaryContent[_binaryBytesSent], _contentLength - _binaryBytesSent, 0);

    if (bytesSent == -1)
      return -1;

    _binaryBytesSent += bytesSent;
    if (_binaryBytesSent >= _contentLength)
      return 0;
    return _binaryBytesSent;
  }

  size_t remaining = _response.length() - static_cast<size_t>(_bytesSent);
  size_t toSend = remaining >= BUFFER_SIZE ? BUFFER_SIZE : remaining;

  bytesSent = send(fd, &_response[_bytesSent], toSend, 0);

  if (bytesSent == -1)
    return -1;

  _bytesSent += bytesSent;

  if (static_cast<size_t>(_bytesSent) >= _response.length())
  {
    // For HEAD requests, don't send binary content
    if (_isBinary && !_binaryContent.empty() && !_isHeadRequest)
    {
      _sendingBinary = true;
      return _bytesSent;
    }
    return 0;
  }
  return _bytesSent;
}
