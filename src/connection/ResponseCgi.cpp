/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ResponseCgi.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 19:00:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 20:08:14 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Response.hpp"

#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <cstring>

#include <sstream>

#include "Request.hpp"

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
  _cgiState.pid = pid;
  _cgiState.stdinFd = pipeIn[1];
  _cgiState.stdoutFd = pipeOut[0];
  _cgiState.inputData = request.getBody();
  _cgiState.inputWritten = 0;
  _cgiState.outputData.clear();
  _cgiState.stdinClosed = false;
  _cgiState.active = true;

  // If no body to write, close stdin immediately
  if (_cgiState.inputData.empty())
  {
    close(_cgiState.stdinFd);
    _cgiState.stdinFd = -1;
    _cgiState.stdinClosed = true;
  }

  return true;
}

void Response::finalizeCgiResponse()
{
  if (!_cgiState.active)
    return;

  // Close remaining pipes
  if (_cgiState.stdinFd >= 0)
  {
    close(_cgiState.stdinFd);
    _cgiState.stdinFd = -1;
  }
  if (_cgiState.stdoutFd >= 0)
  {
    close(_cgiState.stdoutFd);
    _cgiState.stdoutFd = -1;
  }

  // Wait for child
  if (_cgiState.pid > 0)
  {
    int status;
    waitpid(_cgiState.pid, &status, 0);
    _cgiState.pid = -1;
  }

  _cgiState.active = false;

  // Process CGI output and build response
  const std::string &cgiOutput = _cgiState.outputData;
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

        // Ensure headers end with \r\n for proper formatting
        if (normalizedHeaders.size() >= 2 &&
            normalizedHeaders.substr(normalizedHeaders.size() - 2) != "\r\n")
        {
          normalizedHeaders += "\r\n";
        }

        if (normalizedHeaders.find("Content-Length:") == std::string::npos)
        {
          std::ostringstream oss;
          oss << body.length();
          _response = "HTTP/1.1 200 OK\r\n" + normalizedHeaders + "Content-Length: " + oss.str() + "\r\n\r\n" + body;
        }
        else
        {
          _response = "HTTP/1.1 200 OK\r\n" + normalizedHeaders + "\r\n" + body;
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
