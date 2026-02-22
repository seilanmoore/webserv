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

  size_t remaining = state->inputData.size() - state->inputWritten;
  if (remaining > 0)
  {
    size_t toWrite = remaining > CGI_BUFFER_SIZE ? CGI_BUFFER_SIZE : remaining;
    ssize_t n = write(fd, &state->inputData[state->inputWritten], toWrite);
    if (n > 0)
      state->inputWritten += n;
    else if (n < 0)
    {
      close(fd);
      state->stdinFd = -1;
      state->stdinClosed = true;
      _cgiByPipe.erase(fd);
      removePollFD(pos);

      if (state->stdoutClosed || state->stdoutFd < 0)
      {
        finishCgi(state);
      }
      return;
    }
  }

  if (state->inputWritten >= state->inputData.size())
  {
    close(fd);
    state->stdinFd = -1;
    state->stdinClosed = true;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

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

  close(fd);
  state->stdinFd = -1;
  state->stdinClosed = true;
  _cgiByPipe.erase(fd);
  removePollFD(pos);

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

  char buffer[CGI_BUFFER_SIZE + 1];
  ssize_t n = read(fd, buffer, sizeof(buffer));
  if (n > 0)
  {
    state->outputData.append(buffer, n);
  }
  else if (n == 0)
  {
    state->stdoutClosed = true;
    close(fd);
    state->stdoutFd = -1;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

    if (state->stdinClosed || state->stdinFd < 0)
    {
      finishCgi(state);
    }
  }
  else
  {
    state->stdoutClosed = true;
    close(fd);
    state->stdoutFd = -1;
    _cgiByPipe.erase(fd);
    removePollFD(pos);

    if (state->stdinClosed || state->stdinFd < 0)
    {
      finishCgi(state);
    }
  }
}

void Webserv::finishCgi(CgiState *state)
{
  if (state->pid > 0)
  {
    int status;
    waitpid(state->pid, &status, WNOHANG);
    state->pid = -1;
  }

  int connFd = state->connectionFd;
  std::map<int, Connection *>::iterator connIt = _connection.find(connFd);
  if (connIt == _connection.end())
  {
    cleanupCgi(state);
    return;
  }

  Connection *conn = connIt->second;
  Response *resp = conn->getResponsePtr();

  resp->getCgiInfo().outputData = state->outputData;
  resp->getCgiInfo().stdinFd = state->stdinFd;
  resp->getCgiInfo().stdoutFd = state->stdoutFd;
  resp->finalizeCgiResponse();

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
  if (state->stdinFd >= 0)
  {
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

  if (state->pid > 0)
  {
    kill(state->pid, SIGKILL);
    waitpid(state->pid, NULL, 0);
  }

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

  for (size_t i = 0; i < timedOut.size(); ++i)
  {
    timeoutCgi(timedOut[i]);
  }
}

void Webserv::timeoutCgi(CgiState *state)
{
  if (!state)
    return;

  if (state->pid > 0)
  {
    kill(state->pid, SIGKILL);
    waitpid(state->pid, NULL, 0);
    state->pid = -1;
  }

  int connFd = state->connectionFd;
  std::map<int, Connection *>::iterator connIt = _connection.find(connFd);
  if (connIt != _connection.end())
  {
    Connection *conn = connIt->second;
    Response *resp = conn->getResponsePtr();

    std::string body = "<html><body><h1>504 Gateway Timeout - CGI script timed out</h1></body></html>";
    std::ostringstream oss;
    oss << body.length();
    std::string response = "HTTP/1.1 504 Gateway Timeout\r\nContent-Type: text/html\r\nContent-Length: " + oss.str() + "\r\n\r\n" + body;
    resp->setRawResponse(response);

    resp->getCgiInfo().active = false;

    for (nfds_t i = 0; i < _nPollFD; ++i)
    {
      if (_pollFD[i].fd == connFd)
      {
        _pollFD[i].events = POLLOUT;
        break;
      }
    }
  }

  cleanupCgi(state);
}
