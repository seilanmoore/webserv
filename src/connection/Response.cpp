/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:31 by smoore-a          #+#    #+#             */
/*   Updated: 2025/12/28 15:17:38 by smoore-a         ###   ########.fr       */
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
      _chunkOffset(0)
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
    std::string checkMethod = method;
    // HEAD is allowed if GET is allowed (HEAD is GET without body)
    if (method == "HEAD")
      checkMethod = "GET";

    if (location->allowedMethods.find(checkMethod) == location->allowedMethods.end())
    {
      generateErrorPage(405, server);
      return;
    }
  }

  // Check body size limit
  if (request.getContentLength() > server.clientMaxBodySize)
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
    generateErrorPage(403, server);
    return;
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

    if (!_isBinary)
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

  // No index file and no autoindex
  generateErrorPage(403, server);
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
                          "\r\n" +
              _fileContent;
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
                                "\r\n" +
              _fileContent;
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
                                "\r\n" +
              _fileContent;
}

// ============================================================================
// CGI Handler
// ============================================================================

void Response::handleCgi(const Request &request, const std::string &scriptPath,
                         const std::string &interpreter)
{
  // Check if script exists
  if (!fileExists(scriptPath))
  {
    _statusCode = 404;
    _statusMessage = "Not Found";
    _contentType = "text/html";
    _fileContent = "<html><body><h1>404 Not Found - CGI Script</h1></body></html>";
    _contentLength = _fileContent.length();

    std::ostringstream oss;
    oss << _contentLength;
    _response = "HTTP/1.1 404 Not Found\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " +
                oss.str() + "\r\n\r\n" + _fileContent;
    return;
  }

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
    _response = "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " +
                oss.str() + "\r\n\r\n" + _fileContent;
    return;
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
    _response = "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: " +
                oss.str() + "\r\n\r\n" + _fileContent;
    return;
  }

  if (pid == 0)
  {
    // Child process
    close(pipeIn[1]);  // Close write end of stdin pipe
    close(pipeOut[0]); // Close read end of stdout pipe

    dup2(pipeIn[0], STDIN_FILENO);
    dup2(pipeOut[1], STDOUT_FILENO);

    close(pipeIn[0]);
    close(pipeOut[1]);

    // Change to script directory and get script name
    std::string scriptName = scriptPath;
    std::string scriptDir = ".";
    size_t lastSlash = scriptPath.find_last_of('/');
    if (lastSlash != std::string::npos)
    {
      scriptDir = scriptPath.substr(0, lastSlash);
      scriptName = scriptPath.substr(lastSlash + 1);
      chdir(scriptDir.c_str());
    }

    // Build environment variables
    std::vector<std::string> envVars;
    envVars.push_back("REQUEST_METHOD=" + request.getMethod());
    envVars.push_back("QUERY_STRING=" + request.getQueryString());
    envVars.push_back("SCRIPT_FILENAME=" + scriptName);
    envVars.push_back("SCRIPT_NAME=" + request.getPath());
    envVars.push_back("CONTENT_TYPE=" + request.getContentType());

    std::ostringstream contentLenOss;
    contentLenOss << request.getContentLength();
    envVars.push_back("CONTENT_LENGTH=" + contentLenOss.str());

    envVars.push_back("SERVER_PROTOCOL=HTTP/1.1");
    envVars.push_back("SERVER_SOFTWARE=webserv/1.0");
    envVars.push_back("GATEWAY_INTERFACE=CGI/1.1");
    envVars.push_back("PATH_INFO=" + request.getPath());
    envVars.push_back("REDIRECT_STATUS=200");

    // Build envp array
    std::vector<char *> envp;
    for (size_t i = 0; i < envVars.size(); ++i)
      envp.push_back(const_cast<char *>(envVars[i].c_str()));
    envp.push_back(NULL);

    // Build argv
    char interpBuf[256];
    char scriptBuf[256];
    strncpy(interpBuf, interpreter.c_str(), 255);
    interpBuf[255] = '\0';
    strncpy(scriptBuf, scriptName.c_str(), 255);
    scriptBuf[255] = '\0';

    char *argv[3];
    argv[0] = interpBuf;
    argv[1] = scriptBuf;
    argv[2] = NULL;

    execve(argv[0], argv, &envp[0]);
    std::cerr << "CGI exec failed for: " << scriptPath << std::endl;
    _exit(1);
  }
  else
  {
    // Parent process
    close(pipeIn[0]);  // Close read end of stdin pipe
    close(pipeOut[1]); // Close write end of stdout pipe

    // Send POST body to CGI stdin (if any)
    const std::vector<char> &body = request.getBody();
    if (!body.empty())
    {
      write(pipeIn[1], &body[0], body.size());
    }
    close(pipeIn[1]); // Close to signal EOF to child

    // Make stdout pipe non-blocking
    int flags = fcntl(pipeOut[0], F_GETFL, 0);
    fcntl(pipeOut[0], F_SETFL, flags | O_NONBLOCK);

    // Use poll() with timeout to read CGI output
    std::string cgiOutput;
    char buffer[4096];
    struct pollfd pfd;
    pfd.fd = pipeOut[0];
    pfd.events = POLLIN;

    static const int CGI_TIMEOUT_MS = 5000; // 5 second timeout

    while (true)
    {
      int pollRet = poll(&pfd, 1, CGI_TIMEOUT_MS);

      if (pollRet == -1)
      {
        // Poll error
        break;
      }
      else if (pollRet == 0)
      {
        // Timeout - kill the CGI process
        kill(pid, SIGKILL);
        close(pipeOut[0]);
        waitpid(pid, NULL, 0);

        _statusCode = 504;
        _statusMessage = "Gateway Timeout";
        _fileContent = "<html><body><h1>504 Gateway Timeout - CGI script timed out</h1></body></html>";
        _contentType = "text/html";
        _contentLength = _fileContent.length();

        std::ostringstream oss;
        oss << _contentLength;
        _response = "HTTP/1.1 504 Gateway Timeout\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " +
                    oss.str() + "\r\n\r\n" + _fileContent;
        return;
      }

      if (pfd.revents & POLLIN)
      {
        ssize_t n = read(pipeOut[0], buffer, sizeof(buffer) - 1);
        if (n > 0)
        {
          buffer[n] = '\0';
          cgiOutput += buffer;
        }
        else if (n == 0)
        {
          // EOF
          break;
        }
        else
        {
          // n < 0: Read error or would block - poll() already indicated readiness
          // so any error here is a real error, break the loop
          break;
        }
      }

      if (pfd.revents & (POLLHUP | POLLERR))
      {
        // Pipe closed or error
        // Read any remaining data
        ssize_t n;
        while ((n = read(pipeOut[0], buffer, sizeof(buffer) - 1)) > 0)
        {
          buffer[n] = '\0';
          cgiOutput += buffer;
        }
        break;
      }
    }

    close(pipeOut[0]);

    // Wait for child to finish
    int status;
    waitpid(pid, &status, 0);

    if (!cgiOutput.empty())
    {
      // Check if CGI already provides HTTP status line
      if (cgiOutput.find("HTTP/1.") == 0)
      {
        // CGI provides full HTTP response including status line
        _response = cgiOutput;
      }
      else if (cgiOutput.find("Content-Type:") != std::string::npos ||
               cgiOutput.find("Status:") != std::string::npos)
      {
        // CGI provides headers but not status line - prepend HTTP status line
        _response = "HTTP/1.1 200 OK\r\n" + cgiOutput;
      }
      else
      {
        // CGI only provides body
        std::ostringstream oss;
        oss << cgiOutput.length();
        _response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " +
                    oss.str() + "\r\n"
                                "\r\n" +
                    cgiOutput;
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
      _response = "HTTP/1.1 500 Internal Server Error\r\n"
                  "Content-Type: text/html\r\n"
                  "Content-Length: " +
                  oss.str() + "\r\n\r\n" + _fileContent;
    }
  }
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
    if (_isBinary && !_binaryContent.empty())
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
