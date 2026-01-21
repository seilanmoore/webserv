/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   WebservCgi.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 19:30:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:57:35 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <csignal>
#include <sstream>

#include "Connection.hpp"
#include "Response.hpp"
#include "Constants.hpp"

// ============================================================================
// CGI Pipe Handlers
// ============================================================================

void Webserv::handleCgiStdin(nfds_t &pos)
{
  int fd = _pollFD[pos].fd;
  std::map<int, CgiState *>::iterator it = _cgiByPipe.find(fd);
  if (it == _cgiByPipe.end())
    return;

  CgiState *state = it->second;

  // Write data to CGI stdin
  size_t remaining = state->inputData.size() - state->inputWritten;
  if (remaining > 0)
  {
    size_t toWrite = remaining > 65536 ? 65536 : remaining;
    ssize_t n = write(fd, &state->inputData[state->inputWritten], toWrite);
    if (n > 0)
      state->inputWritten += n;
    else if (n < 0)
    {
      // Write error - close stdin pipe to let CGI process what it has
      close(fd);
      state->stdinFd = -1;
      state->stdinClosed = true;
      _cgiByPipe.erase(fd);
      removePollFD(pos);

      // Check if CGI is complete (stdout already closed)
      if (state->stdoutClosed || state->stdoutFd < 0)
      {
        finishCgi(state);
      }
      return;
    }
    // n == 0 is unusual for write, treat as needing another poll cycle
  }

  // Check if all data written
  if (state->inputWritten >= state->inputData.size())
  {
    close(fd);
    state->stdinFd = -1;
    state->stdinClosed = true;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

    // Check if CGI is complete (stdout already closed)
    if (state->stdoutClosed || state->stdoutFd < 0)
    {
      finishCgi(state);
    }
  }
}

void Webserv::handleCgiStdinError(nfds_t &pos)
{
  int fd = _pollFD[pos].fd;
  std::map<int, CgiState *>::iterator it = _cgiByPipe.find(fd);
  if (it == _cgiByPipe.end())
    return;

  CgiState *state = it->second;

  // Close stdin pipe on error - CGI will process what it has received
  close(fd);
  state->stdinFd = -1;
  state->stdinClosed = true;
  _cgiByPipe.erase(fd);
  removePollFD(pos);

  // Check if CGI is complete (stdout already closed)
  if (state->stdoutClosed || state->stdoutFd < 0)
  {
    finishCgi(state);
  }
}

void Webserv::handleCgiStdout(nfds_t &pos)
{
  int fd = _pollFD[pos].fd;
  std::map<int, CgiState *>::iterator it = _cgiByPipe.find(fd);
  if (it == _cgiByPipe.end())
    return;

  CgiState *state = it->second;

  // Read data from CGI stdout
  char buffer[65536];
  ssize_t n = read(fd, buffer, sizeof(buffer));
  if (n > 0)
  {
    state->outputData.append(buffer, n);
  }
  else if (n == 0) // EOF - CGI finished
  {
    state->stdoutClosed = true;
    close(fd);
    state->stdoutFd = -1;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

    // Check if CGI is complete (both pipes closed)
    if (state->stdinClosed || state->stdinFd < 0)
    {
      finishCgi(state);
    }
  }
  else // n < 0: read error - close and finish CGI with what we have
  {
    state->stdoutClosed = true;
    close(fd);
    state->stdoutFd = -1;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

    // Check if CGI is complete (both pipes closed)
    if (state->stdinClosed || state->stdinFd < 0)
    {
      finishCgi(state);
    }
  }
}

void Webserv::finishCgi(CgiState *state)
{
  // Wait for child process
  if (state->pid > 0)
  {
    int status;
    waitpid(state->pid, &status, WNOHANG);
    state->pid = -1;
  }

  // Get connection and response
  int connFd = state->connectionFd;
  std::map<int, Connection *>::iterator connIt = _connection.find(connFd);
  if (connIt == _connection.end())
  {
    cleanupCgi(state);
    return;
  }

  Connection *conn = connIt->second;
  Response *resp = conn->getResponsePtr();

  // Copy output to response's CGI info and finalize
  resp->getCgiInfo().outputData = state->outputData;
  resp->finalizeCgiResponse();

  // Re-enable connection for sending response
  for (nfds_t i = 0; i < _nPollFD; ++i)
  {
    if (_pollFD[i].fd == connFd)
    {
      _pollFD[i].events = POLLOUT;
      break;
    }
  }

  cleanupCgi(state);
}

void Webserv::cleanupCgi(CgiState *state)
{
  // Close any remaining pipes (only if not already closed)
  if (state->stdinFd >= 0)
  {
    // Remove from poll if still there
    for (nfds_t i = 0; i < _nPollFD; ++i)
    {
      if (_pollFD[i].fd == state->stdinFd)
      {
        close(_pollFD[i].fd);
        _pollFD.erase(_pollFD.begin() + i);
        _pollFDType.erase(_pollFDType.begin() + i);
        --_nPollFD;
        break;
      }
    }
    _cgiByPipe.erase(state->stdinFd);
    state->stdinFd = -1;
  }
  if (state->stdoutFd >= 0)
  {
    // Remove from poll if still there
    for (nfds_t i = 0; i < _nPollFD; ++i)
    {
      if (_pollFD[i].fd == state->stdoutFd)
      {
        close(_pollFD[i].fd);
        _pollFD.erase(_pollFD.begin() + i);
        _pollFDType.erase(_pollFDType.begin() + i);
        --_nPollFD;
        break;
      }
    }
    _cgiByPipe.erase(state->stdoutFd);
    state->stdoutFd = -1;
  }

  // Kill process if still running
  if (state->pid > 0)
  {
    kill(state->pid, SIGKILL);
    waitpid(state->pid, NULL, 0);
  }

  // Remove from maps
  _cgiByConnection.erase(state->connectionFd);

  delete state;
}
// ============================================================================
// CGI Timeout Handling
// ============================================================================

void Webserv::checkCgiTimeouts()
{
  if (_cgiByConnection.empty())
    return;

  std::vector<CgiState *> timedOut;

  // Increment poll cycles and collect timed out CGIs
  for (std::map<int, CgiState *>::iterator it = _cgiByConnection.begin();
       it != _cgiByConnection.end(); ++it)
  {
    CgiState *cgi = it->second;
    if (cgi && cgi->active)
    {
      cgi->pollCycles++;
      if (cgi->pollCycles >= CGI_TIMEOUT_CYCLES)
      {
        timedOut.push_back(cgi);
      }
    }
  }

  // Handle timed out CGIs
  for (size_t i = 0; i < timedOut.size(); ++i)
  {
    timeoutCgi(timedOut[i]);
  }
}

void Webserv::timeoutCgi(CgiState *state)
{
  if (!state)
    return;

  // Kill the CGI process
  if (state->pid > 0)
  {
    kill(state->pid, SIGKILL);
    waitpid(state->pid, NULL, 0);
    state->pid = -1;
  }

  // Get connection and response to send 504 error
  int connFd = state->connectionFd;
  std::map<int, Connection *>::iterator connIt = _connection.find(connFd);
  if (connIt != _connection.end())
  {
    Connection *conn = connIt->second;
    Response *resp = conn->getResponsePtr();

    // Set 504 Gateway Timeout response
    std::string body = "<html><body><h1>504 Gateway Timeout - CGI script timed out</h1></body></html>";
    std::ostringstream oss;
    oss << body.length();
    std::string response = "HTTP/1.1 504 Gateway Timeout\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + body;
    resp->setRawResponse(response);

    // Mark CGI as inactive
    resp->getCgiInfo().active = false;

    // Re-enable connection for sending response
    for (nfds_t i = 0; i < _nPollFD; ++i)
    {
      if (_pollFD[i].fd == connFd)
      {
        _pollFD[i].events = POLLOUT;
        break;
      }
    }
  }

  // Cleanup the CGI state
  cleanupCgi(state);
}