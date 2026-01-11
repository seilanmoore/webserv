/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:31 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/11 18:55:28 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <poll.h>
#include <fcntl.h>
#include <signal.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

#include "Webserv.hpp"
#include "Request.hpp"
#include "Config.hpp"
#include "CgiHandler.hpp"
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

void Response::generateResponse(const Request &request, const ServerConfig &server)
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

void Response::handleGet(const Request &request, const ServerConfig &server,
                         const LocationConfig *location)
{
  std::string uri = request.getPath();
  std::string root = location ? location->root : server.root;
  std::string locationPath = location ? location->path : "/";

  // Build full file path
  std::string fullPath = buildFullPath(root, uri, locationPath);

  // Check if path exists
  if (!fileExists(fullPath))
  {
    generateErrorPage(404, server);
    return;
  }

  // Handle directory
  if (isDirectory(fullPath))
  {
    serveDirectory(fullPath, uri, location, server);
    return;
  }

  // Serve file
  serveFile(fullPath);
}

void Response::handlePost(const Request &request, const ServerConfig &server,
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
      std::string fullPath = buildFullPath(root, uri, locationPath);
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

  std::string fullPath = buildFullPath(root, uri, locationPath);

  // Check if file exists
  if (!fileExists(fullPath))
  {
    generateErrorPage(404, server);
    return;
  }

  // Don't allow deleting directories
  if (isDirectory(fullPath))
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
  _contentType = getMimeType(filePath);
  _filename = filePath;

  // Check if binary
  _isBinary = (_contentType.find("text/") == std::string::npos &&
               _contentType.find("application/json") == std::string::npos &&
               _contentType.find("application/javascript") == std::string::npos &&
               _contentType.find("application/xml") == std::string::npos);

  // Threshold for chunked encoding (10 MB - for really large files)
  static const size_t CHUNKED_THRESHOLD = 10485760;

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

    if (fileExists(indexPath) && !isDirectory(indexPath))
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
  _statusMessage = getStatusMessage(code);
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
  _statusMessage = getStatusMessage(code);
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
// CGI Handler (now non-blocking - starts CGI and returns immediately)
// ============================================================================

void Response::handleCgi(const Request &request, const std::string &scriptPath,
                         const std::string &interpreter)
{
  // Start CGI process - returns immediately, CGI runs asynchronously
  // The actual response will be built later when CGI completes
  startCgi(request, scriptPath, interpreter);
}

// ============================================================================
// Non-blocking CGI functions
// ============================================================================

bool Response::startCgi(const Request &request, const std::string &scriptPath,
                        const std::string &interpreter)
{
  // Create pipes: one for stdin (to send body), one for stdout (to receive response)
  int pipeIn[2];  // Parent writes, child reads (stdin)
  int pipeOut[2]; // Child writes, parent reads (stdout)

  if (pipe(pipeIn) == -1 || pipe(pipeOut) == -1)
  {
    _statusCode = 500;
    _statusMessage = "Internal Server Error";
    _fileContent = "<html><body><h1>500 Internal Server Error - CGI pipe failed</h1></body></html>";
    _contentType = "text/html";
    _contentLength = _fileContent.length();
    std::ostringstream oss;
    oss << _contentLength;
    _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + _fileContent;
    return false;
  }

  pid_t pid = fork();
  if (pid == -1)
  {
    close(pipeIn[0]);
    close(pipeIn[1]);
    close(pipeOut[0]);
    close(pipeOut[1]);
    _statusCode = 500;
    _statusMessage = "Internal Server Error";
    _fileContent = "<html><body><h1>500 Internal Server Error - CGI fork failed</h1></body></html>";
    _contentType = "text/html";
    _contentLength = _fileContent.length();
    std::ostringstream oss;
    oss << _contentLength;
    _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + _fileContent;
    return false;
  }

  if (pid == 0)
  {
    // Child process
    close(pipeIn[1]);
    close(pipeOut[0]);
    dup2(pipeIn[0], STDIN_FILENO);
    dup2(pipeOut[1], STDOUT_FILENO);
    close(pipeIn[0]);
    close(pipeOut[1]);

    // Get absolute path of interpreter
    std::string absInterpreter = interpreter;
    if (!interpreter.empty() && interpreter[0] != '/')
    {
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)))
        absInterpreter = std::string(cwd) + "/" + interpreter;
    }

    // Change to script directory
    std::string scriptName = scriptPath;
    std::string scriptDir = ".";
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash != std::string::npos)
    {
      scriptDir = scriptPath.substr(0, lastSlash);
      scriptName = scriptPath.substr(lastSlash + 1);
      chdir(scriptDir.c_str());
    }

    // Build environment
    std::vector<std::string> envVars;
    envVars.push_back("REQUEST_METHOD=" + request.getMethod());
    envVars.push_back("QUERY_STRING=" + request.getQueryString());
    envVars.push_back("SCRIPT_FILENAME=" + scriptName);
    envVars.push_back("CONTENT_TYPE=" + request.getContentType());
    std::ostringstream clOss;
    clOss << request.getContentLength();
    envVars.push_back("CONTENT_LENGTH=" + clOss.str());
    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("PATH_INFO=" + request.getPath());
    envVars.push_back("REDIRECT_STATUS=200");

    // Add HTTP headers
    const std::map<std::string, std::string> &headers = request.getOtherHeaders();
    for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
    {
      std::string envName = "HTTP_";
      for (size_t i = 0; i < it->first.size(); ++i)
      {
        char c = it->first[i];
        envName += (c == '-') ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
      }
      envVars.push_back(envName + "=" + it->second);
    }

    std::vector<char *> envp;
    for (size_t i = 0; i < envVars.size(); ++i)
      envp.push_back(const_cast<char *>(envVars[i].c_str()));
    envp.push_back(NULL);

    char interpBuf[1024], scriptBuf[256];
    strncpy(interpBuf, absInterpreter.c_str(), 1023);
    interpBuf[1023] = '\0';
    strncpy(scriptBuf, scriptName.c_str(), 255);
    scriptBuf[255] = '\0';

    char *argv[3] = {interpBuf, scriptBuf, NULL};
    execve(argv[0], argv, &envp[0]);
    _exit(1);
  }

  // Parent process - set up non-blocking CGI state
  close(pipeIn[0]);
  close(pipeOut[1]);

  // Make pipes non-blocking (only F_SETFL and O_NONBLOCK allowed per subject)
  fcntl(pipeIn[1], F_SETFL, O_NONBLOCK);
  fcntl(pipeOut[0], F_SETFL, O_NONBLOCK);

  // Initialize CGI state
  _cgiInfo.pid = pid;
  _cgiInfo.stdinFd = pipeIn[1];
  _cgiInfo.stdoutFd = pipeOut[0];
  _cgiInfo.inputData = request.getBody();
  _cgiInfo.inputWritten = 0;
  _cgiInfo.outputData.clear();
  _cgiInfo.stdinClosed = false;
  _cgiInfo.active = true;

  // If no body to write, close stdin immediately
  if (_cgiInfo.inputData.empty())
  {
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
    _cgiInfo.stdinClosed = true;
  }

  return true;
}

bool Response::writeCgiInput()
{
  if (!_cgiInfo.active || _cgiInfo.stdinClosed || _cgiInfo.stdinFd < 0)
    return false;

  size_t remaining = _cgiInfo.inputData.size() - _cgiInfo.inputWritten;
  if (remaining == 0)
  {
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
    _cgiInfo.stdinClosed = true;
    return false;
  }

  size_t toWrite = remaining > 65536 ? 65536 : remaining;
  ssize_t n = write(_cgiInfo.stdinFd, &_cgiInfo.inputData[_cgiInfo.inputWritten], toWrite);
  if (n > 0)
    _cgiInfo.inputWritten += n;
  else if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
  {
    // Write error, close stdin
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
    _cgiInfo.stdinClosed = true;
    return false;
  }

  remaining = _cgiInfo.inputData.size() - _cgiInfo.inputWritten;
  if (remaining == 0)
  {
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
    _cgiInfo.stdinClosed = true;
    return false;
  }
  return true;
}

bool Response::readCgiOutput()
{
  if (!_cgiInfo.active || _cgiInfo.stdoutFd < 0)
    return false;

  char buffer[65536];
  ssize_t n = read(_cgiInfo.stdoutFd, buffer, sizeof(buffer));
  if (n > 0)
  {
    _cgiInfo.outputData.append(buffer, n);
    return true;
  }
  else if (n == 0)
  {
    // EOF - CGI finished
    return false;
  }
  else if (errno == EAGAIN || errno == EWOULDBLOCK)
  {
    // No data available yet
    return true;
  }
  // Read error
  return false;
}

void Response::finalizeCgiResponse()
{
  if (!_cgiInfo.active)
    return;

  // Close remaining pipes
  if (_cgiInfo.stdinFd >= 0)
  {
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
  }
  if (_cgiInfo.stdoutFd >= 0)
  {
    close(_cgiInfo.stdoutFd);
    _cgiInfo.stdoutFd = -1;
  }

  // Wait for child
  if (_cgiInfo.pid > 0)
  {
    int status;
    waitpid(_cgiInfo.pid, &status, 0);
    _cgiInfo.pid = -1;
  }

  _cgiInfo.active = false;

  // Process CGI output and build response
  const std::string &cgiOutput = _cgiInfo.outputData;
  if (!cgiOutput.empty())
  {
    if (cgiOutput.find("HTTP/1.") == 0)
    {
      _response = cgiOutput;
    }
    else if (cgiOutput.find("Content-Type:") != std::string::npos ||
             cgiOutput.find("Status:") != std::string::npos)
    {
      size_t headerEnd = cgiOutput.find("\r\n\r\n");
      size_t sepLen = 4;
      if (headerEnd == std::string::npos)
      {
        headerEnd = cgiOutput.find("\n\n");
        sepLen = 2;
      }
      if (headerEnd != std::string::npos)
      {
        std::string headers = cgiOutput.substr(0, headerEnd);
        std::string body = cgiOutput.substr(headerEnd + sepLen);

        std::string normalizedHeaders;
        for (size_t i = 0; i < headers.size(); ++i)
        {
          if (headers[i] == '\n' && (i == 0 || headers[i - 1] != '\r'))
            normalizedHeaders += "\r\n";
          else
            normalizedHeaders += headers[i];
        }

        if (normalizedHeaders.find("Content-Length:") == std::string::npos)
        {
          std::ostringstream oss;
          oss << body.length();
          _response = "HTTP/1.1 200 OK\r\n" + normalizedHeaders + "\r\nContent-Length: " + oss.str() + "\r\n\r\n" + body;
        }
        else
        {
          _response = "HTTP/1.1 200 OK\r\n" + normalizedHeaders + "\r\n\r\n" + body;
        }
      }
      else
      {
        _response = "HTTP/1.1 200 OK\r\n" + cgiOutput;
      }
    }
    else
    {
      std::ostringstream oss;
      oss << cgiOutput.length();
      _response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + cgiOutput;
    }
    _fileFound = true;
  }
  else
  {
    _statusCode = 500;
    _statusMessage = "Internal Server Error";
    _fileContent = "<html><body><h1>500 Internal Server Error - CGI produced no output</h1></body></html>";
    _contentType = "text/html";
    _contentLength = _fileContent.length();
    std::ostringstream oss;
    oss << _contentLength;
    _response = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + _fileContent;
  }
}

void Response::killCgi()
{
  if (!_cgiInfo.active)
    return;

  if (_cgiInfo.stdinFd >= 0)
  {
    close(_cgiInfo.stdinFd);
    _cgiInfo.stdinFd = -1;
  }
  if (_cgiInfo.stdoutFd >= 0)
  {
    close(_cgiInfo.stdoutFd);
    _cgiInfo.stdoutFd = -1;
  }
  if (_cgiInfo.pid > 0)
  {
    kill(_cgiInfo.pid, SIGKILL);
    waitpid(_cgiInfo.pid, NULL, 0);
    _cgiInfo.pid = -1;
  }
  _cgiInfo.active = false;
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

    // Format chunk: <size in hex>\r\n<data>\r\n
    std::ostringstream chunkHeader;
    chunkHeader << std::hex << chunkSize << "\r\n";
    std::string header = chunkHeader.str();

    // Send chunk header
    bytesSent = send(fd, header.c_str(), header.length(), 0);
    if (bytesSent == -1)
      return -1;

    // Send chunk data
    bytesSent = send(fd, contentData + _chunkOffset, chunkSize, 0);
    if (bytesSent == -1)
      return -1;

    // Send chunk trailer
    bytesSent = send(fd, "\r\n", 2, 0);
    if (bytesSent == -1)
      return -1;

    _chunkOffset += chunkSize;
    return static_cast<ssize_t>(_chunkOffset);
  }

  // Regular (non-chunked) response handling
  if (_sendingBinary)
  {
    if (_contentLength - _binaryBytesSent >= BUFFER_SIZE)
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

const std::string &Response::getResponse() const
{
  return _response;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string Response::getStatusMessage(int code) const
{
  switch (code)
  {
  case 200:
    return "OK";
  case 201:
    return "Created";
  case 204:
    return "No Content";
  case 301:
    return "Moved Permanently";
  case 302:
    return "Found";
  case 304:
    return "Not Modified";
  case 307:
    return "Temporary Redirect";
  case 308:
    return "Permanent Redirect";
  case 400:
    return "Bad Request";
  case 403:
    return "Forbidden";
  case 404:
    return "Not Found";
  case 405:
    return "Method Not Allowed";
  case 413:
    return "Payload Too Large";
  case 414:
    return "URI Too Long";
  case 500:
    return "Internal Server Error";
  case 501:
    return "Not Implemented";
  case 502:
    return "Bad Gateway";
  case 504:
    return "Gateway Timeout";
  default:
    return "Unknown";
  }
}

std::string Response::getMimeType(const std::string &path) const
{
  size_t dot = path.find_last_of('.');
  if (dot == std::string::npos)
    return "application/octet-stream";

  std::string ext = path.substr(dot);

  // Text types
  if (ext == ".html" || ext == ".htm")
    return "text/html";
  if (ext == ".css")
    return "text/css";
  if (ext == ".js")
    return "application/javascript";
  if (ext == ".json")
    return "application/json";
  if (ext == ".xml")
    return "application/xml";
  if (ext == ".txt")
    return "text/plain";

  // Image types
  if (ext == ".png")
    return "image/png";
  if (ext == ".jpg" || ext == ".jpeg")
    return "image/jpeg";
  if (ext == ".gif")
    return "image/gif";
  if (ext == ".ico")
    return "image/x-icon";
  if (ext == ".svg")
    return "image/svg+xml";
  if (ext == ".webp")
    return "image/webp";

  // Audio/Video
  if (ext == ".mp3")
    return "audio/mpeg";
  if (ext == ".mp4")
    return "video/mp4";
  if (ext == ".webm")
    return "video/webm";

  // Documents
  if (ext == ".pdf")
    return "application/pdf";
  if (ext == ".zip")
    return "application/zip";

  return "application/octet-stream";
}

std::string Response::buildFullPath(const std::string &root, const std::string &uri,
                                    const std::string &locationPath) const
{
  std::string path = root;

  // Remove trailing slash from root if present
  if (!path.empty() && path[path.length() - 1] == '/')
    path.erase(path.length() - 1);

  // Get the part of URI after the location path
  std::string relativePath = uri;
  if (locationPath != "/" && uri.find(locationPath) == 0)
  {
    relativePath = uri.substr(locationPath.length());
    // If root already contains the location-like path, don't add uri part that would duplicate
    // e.g., root="./cgi-bin", location="/cgi-bin", uri="/cgi-bin/fortune.py"
    // We want: ./cgi-bin/fortune.py, not ./cgi-bin/cgi-bin/fortune.py
  }

  // Ensure relative path starts with /
  if (!relativePath.empty() && relativePath[0] != '/')
    relativePath = "/" + relativePath;

  return path + relativePath;
}

bool Response::isDirectory(const std::string &path) const
{
  struct stat st;
  if (stat(path.c_str(), &st) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

bool Response::fileExists(const std::string &path) const
{
  struct stat st;
  return (stat(path.c_str(), &st) == 0);
}
