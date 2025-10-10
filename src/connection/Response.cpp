/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:31 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 13:44:22 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <cstdlib>

#include <fstream>
#include <sstream>
#include <vector>

#include "Webserv.hpp"
#include "Request.hpp"
#include "utils.hpp"

// #include "CgiHandler.hpp"

Response::Response()
    : _sendingBinary(false),
      _binaryBytesSent(0),

      //_buffer(),
      _bytesSent(0),

      _contentType(),

      _filename(),
      _fileFound(false),

      // std::ifstream _file(0),
      _fileContent(),

      _isBinary(false),
      // std::ifstream _binaryFile(0),
      _binaryContent(),

      _contentLength(0),

      _response()
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

// void Response::setStatusLine();

void Response::generateResponse(const Request &request)
{
  _contentType = request.getContentType();
  _filename = request.getFilename();
  _isBinary = request.getIsBinary();

  // // Check if this is a CGI script request
  // if (isCgiScript(_filename))
  // {
  //   // For CGI, we need to build the full script path
  //   std::string scriptPath = _filename;

  //   // Extract query string and method from request
  //   std::string queryString = request.getQueryString();
  //   std::string method = request.getMethod();

  //   // Create a pipe to capture CGI output
  //   int pipefd[2];
  //   if (pipe(pipefd) == -1)
  //   {
  //     _fileContent = "<html><body><h1>500 Internal Server Error - CGI pipe failed</h1></body></html>";
  //     _contentType = "text/html";
  //     _isBinary = false;
  //     _fileFound = true;
  //   }
  //   else
  //   {
  //     pid_t pid = fork();
  //     if (pid == -1)
  //     {
  //       _fileContent = "<html><body><h1>500 Internal Server Error - CGI fork failed</h1></body></html>";
  //       _contentType = "text/html";
  //       _isBinary = false;
  //       _fileFound = true;
  //       close(pipefd[0]);
  //       close(pipefd[1]);
  //     }
  //     else if (pid == 0)
  //     {
  //       // Child process - execute CGI
  //       close(pipefd[0]);               // Close read end
  //       dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
  //       close(pipefd[1]);

  //       char *envp[4];
  //       char interp_buf[256];
  //       char script_buf[256];
  //       char *argv[3];

  //       buildCgiEnv(scriptPath, queryString, method, envp);
  //       buildCgiExecArgBuffers(scriptPath, argv, interp_buf, script_buf);
  //       execve(argv[0], argv, envp);

  //       // If exec fails
  //       std::cerr << "CGI exec failed for: " << scriptPath << std::endl;
  //       exit(1);
  //     }
  //     else
  //     {
  //       // Parent process - read CGI output
  //       close(pipefd[1]); // Close write end

  //       std::string cgiOutput;
  //       char buffer[4096];
  //       ssize_t n;

  //       while ((n = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
  //       {
  //         buffer[n] = '\0';
  //         cgiOutput += buffer;
  //       }

  //       close(pipefd[0]);
  //       waitpid(pid, NULL, 0);

  //       if (!cgiOutput.empty())
  //       {
  //         // CGI scripts output their own HTTP headers
  //         _response = cgiOutput;
  //         _fileFound = true;
  //         return; // Early return for CGI - response is complete
  //       }
  //       else
  //       {
  //         _fileContent = "<html><body><h1>500 Internal Server Error - CGI produced no output</h1></body></html>";
  //         _contentType = "text/html";
  //         _isBinary = false;
  //         _fileFound = true;
  //       }
  //     }
  //   }
  // }
  // else
  // {
  // Handle static files as before
  if (_isBinary)
  {
    std::ifstream file(_filename.c_str(), std::ios::in | std::ios::binary);
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
    }
  }
  else
  {
    std::ifstream file(_filename.c_str());
    if (file.is_open())
    {
      std::stringstream buffer_stream;
      buffer_stream << file.rdbuf();
      _fileContent = buffer_stream.str();
      file.close();
      _fileFound = true;
    }
  }
  //}

  if (!_fileFound)
  {
    _fileContent = "<html><body><h1>File not found</h1></body></html>";
    _contentType = "text/html";
    _isBinary = false;
  }

  std::ostringstream oss;
  _contentLength = _isBinary ? _binaryContent.size() : _fileContent.length();
  oss << _contentLength;
  std::string length_str = oss.str();

  _response =
      "HTTP/1.1 200 OK\r\n"
      "Content-Type: " +
      _contentType + "\r\n"
                     "Content-Length: " +
      length_str + "\r\n"
                   "\r\n" +
      _fileContent;
}

ssize_t Response::sendResponse(int fd)
{
  ssize_t bytesSent;

  if (_sendingBinary)
  {
    if (_contentLength - _binaryBytesSent >= BUFFER_SIZE)
      bytesSent = send(fd, &_binaryContent[_binaryBytesSent], BUFFER_SIZE, 0);
    else
      bytesSent = send(fd, &_binaryContent[_binaryBytesSent], _contentLength - _binaryBytesSent, 0);

    if (bytesSent == -1)
    {
      DEBUG_PRINT(errorStr(errno));
      return -1;
    }

    _binaryBytesSent += bytesSent;
    if (_binaryBytesSent == _contentLength)
      return 0;
    return _binaryBytesSent;
  }

  if (_response.length() - _bytesSent >= BUFFER_SIZE)
    bytesSent = send(fd, &_response[_bytesSent], BUFFER_SIZE, 0);
  else
    bytesSent = send(fd, &_response[_bytesSent], _response.length() - _bytesSent, 0);

  if (bytesSent == -1)
  {
    DEBUG_PRINT(errorStr(errno));
    return -1;
  }

  std::string responseSent(_response.begin() + _bytesSent, _response.begin() + _bytesSent + bytesSent);
  // std::cerr << "Bytes sent: " << bytesSent << '\n';
  // std::cerr << "Response sent: " << responseSent << '\n';

  _bytesSent += bytesSent;
  if (static_cast<size_t>(_bytesSent) == _response.length())
  {
    if (_isBinary)
    {
      _sendingBinary = true;
      return _bytesSent;
    }
    return 0;
  }
  return _bytesSent;

  // if (_isBinary)
  // {
  //   if (send(fd, _response.c_str(), _response.length(), 0) == -1)
  //   {
  //     DEBUG_PRINT(errorStr(errno));
  //     return -1;
  //   }
  //   if (_contentLength > 0)
  //   {
  //     if (send(fd, &_binaryContent[0], _contentLength, 0) == -1)
  //     {
  //       DEBUG_PRINT(errorStr(errno));
  //       return -1;
  //     }
  //   }
  //   std::cerr << "Binary response sent\n"
  //             << _response << '\n';
  // }
  // else
  // {
  //   _response += _fileContent;
  //   if (send(fd, _response.c_str(), _response.length(), 0) == -1)
  //   {
  //     DEBUG_PRINT(errorStr(errno));
  //     return;
  //   }
  //   std::cerr << "Response sent:\n"
  //             << _response << '\n';
  // }
}

const std::string &Response::getResponse() const
{
  return _response;
}
