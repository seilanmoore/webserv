/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:55:34 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/11 19:06:42 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "Config.hpp"
#include "Server.hpp"
#include "Connection.hpp"
#include "Response.hpp"
#include "utils.hpp"

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

static void printPoll(nfds_t nPollFD, const std::vector<struct pollfd> &pollFD, const std::vector<ePollFDType> &type)
{
  std::cerr << "POLL FD\n";
  std::cerr << "-------\n";
  for (nfds_t i = 0; i < nPollFD; ++i)
  {
    std::cerr << (type[i] == SERVER ? "Server" : "Connection") << ": " << pollFD[i].fd << '\n';
  }
}

///////////////////////////////////////////
///////////////////////////////////////////
///////////////////////////////////////////

Webserv::Webserv()
    : _pollFD(),
      _pollFDType(),
      _nPollFD(0),
      _server(),
      _serverConfigIndex(),
      _nRunningServer(0),
      _connection(),
      _nConnection(0)

{
}

Webserv::~Webserv()
{
  printPoll(_nPollFD, _pollFD, _pollFDType);

  while (_nRunningServer)
    deleteServer(_server.begin()->first);

  for (nfds_t i = 0; i < _nPollFD; ++i)
    close(_pollFD[i].fd);
}

void Webserv::parse(int argc, char **argv)
{
  std::string configFile = DEFAULT_CONFIG_FILE_PATH;

  if (argc == 1)
    ; // Use default config path
  else if (argc == 2)
    configFile = argv[1];
  else
    throw std::runtime_error("Usage: ./webserv [configuration file]");

  _config.parse(configFile);
  std::cerr << "Configuration loaded: " << _config.getServerCount() << " server(s)" << std::endl;
}

struct conf
{
  std::string name;
  uint16_t port;
};

void Webserv::initServer()
{
  const std::vector<ServerConfig> &servers = _config.getServers();

  for (size_t i = 0; i < servers.size(); ++i)
  {
    struct conf c;
    c.name = servers[i].serverName.empty() ? "server" + static_cast<std::ostringstream &>(std::ostringstream() << i).str() : servers[i].serverName;
    c.port = servers[i].port;
    int fd = addServer(c);
    if (fd != -1)
      _serverConfigIndex[fd] = i;
  }

  _nPollFD = static_cast<nfds_t>(_nRunningServer);
}

void Webserv::loop()
{
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  // SIGPIPE is ignored in main() to prevent crash on broken pipe
  // signal(SIGPIPE, signalHandler);
  int pollRet;
  while (true)
  {
    pollRet = poll(_pollFD.data(), _nPollFD, 5000);

    if (pollRet < 0)
    {
      DEBUG_PRINT(errorStr(errno));
      break;
    }

    if (pollRet == 0)
    {
      // Timeout occurred, no events - continue polling
      continue;
    }

    checkFDReturnedEvents();
  }
}

void Webserv::checkFDReturnedEvents()
{
  nfds_t currentNFD = _nPollFD;
  for (nfds_t pos = 0; pos < currentNFD; ++pos)
  {
    if (_pollFD[pos].revents & (POLLERR | POLLHUP | POLLNVAL))
    {
      if (_pollFDType[pos] == SERVER)
        restartServer(pos);
      else if (_pollFDType[pos] == CONNECTION)
        deleteConnection(pos);
      else if (_pollFDType[pos] == CGI_STDIN || _pollFDType[pos] == CGI_STDOUT)
      {
        // CGI pipe error/close
        if (_pollFDType[pos] == CGI_STDOUT)
          handleCgiStdout(pos); // Try to read remaining data
      }
    }
    else if (_pollFD[pos].revents & POLLIN)
    {
      if (_pollFDType[pos] == SERVER)
        addConnection(_pollFD[pos].fd);
      else if (_pollFDType[pos] == CONNECTION)
        receiveConnectionRequest(pos);
      else if (_pollFDType[pos] == CGI_STDOUT)
        handleCgiStdout(pos);
    }
    else if (_pollFD[pos].revents & POLLOUT)
    {
      if (_pollFDType[pos] == CGI_STDIN)
        handleCgiStdin(pos);
      else
        sendServerResponse(pos);
    }

    if (currentNFD > _nPollFD)
      currentNFD = _nPollFD;
  }
}

void Webserv::receiveConnectionRequest(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  Connection *connection = _connection[connectionFD];

  ssize_t receiveStatus = connection->receiveRequest();

  if (receiveStatus == -1)
  {
    deleteConnection(pos);
    return;
  }

  if (receiveStatus == 0)
  {
    // Get the server config for this connection
    int serverFD = connection->getServerFD();
    size_t configIndex = _serverConfigIndex[serverFD];
    const ServerConfig &serverConfig = _config.getServer(configIndex);

    connection->generateResponse(serverConfig);

    // Check if a CGI was started
    if (connection->hasPendingCgi())
    {
      // Add CGI pipes to poll
      Response *resp = connection->getResponsePtr();
      CgiInfo &cgi = resp->getCgiInfo();

      // Create CGI state
      CgiState *state = new CgiState();
      state->pid = cgi.pid;
      state->stdinFd = cgi.stdinFd;
      state->stdoutFd = cgi.stdoutFd;
      state->connectionFd = connectionFD;
      state->inputData = cgi.inputData;
      state->inputWritten = cgi.inputWritten;
      state->outputData = cgi.outputData;
      state->stdinClosed = cgi.stdinClosed;
      state->stdoutClosed = false;

      _cgiByConnection[connectionFD] = state;

      // Add stdin pipe if we have data to write
      if (!state->stdinClosed && state->stdinFd >= 0)
      {
        struct pollfd pfd;
        pfd.fd = state->stdinFd;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        _pollFD.push_back(pfd);
        _pollFDType.push_back(CGI_STDIN);
        _cgiByPipe[state->stdinFd] = state;
        ++_nPollFD;
      }

      // Add stdout pipe
      if (state->stdoutFd >= 0)
      {
        struct pollfd pfd;
        pfd.fd = state->stdoutFd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        _pollFD.push_back(pfd);
        _pollFDType.push_back(CGI_STDOUT);
        _cgiByPipe[state->stdoutFd] = state;
        ++_nPollFD;
      }

      // Don't change connection to POLLOUT yet - wait for CGI to complete
      _pollFD[pos].events = 0; // Remove from poll temporarily
    }
    else
    {
      _pollFD[pos].events = POLLOUT;
    }
  }
}

void Webserv::sendServerResponse(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  Connection *conn = _connection[connectionFD];
  ssize_t sendStatus = conn->sendServerResponse();
  if (sendStatus == -1)
  {
    DEBUG_VAR(errorStr(errno), _pollFD[pos].fd);
    deleteConnection(pos);
  }
  else if (sendStatus == 0)
  {
    // Debug output commented for production
    // std::cerr << "Response: " << conn->getResponse();

    // Check if client requested Connection: close
    if (!conn->shouldKeepAlive())
    {
      deleteConnection(pos);
      return;
    }

    _pollFD[pos].events = POLLIN;
    conn->resetRequestResponse();
  }

  //   DEBUG_VAR("Error: send() returned 0", _pollFD[pos].fd);
  //   deleteConnection(pos);
}

void Webserv::addPollFD(int fd, tPollFDType fdType)
{
  struct pollfd pollFD;
  pollFD.fd = fd;
  pollFD.events = POLLIN;
  pollFD.revents = 0;
  _pollFD.push_back(pollFD);
  _pollFDType.push_back(fdType);
  ++_nPollFD;
}

void Webserv::deletePollFD(nfds_t &pos)
{
  close(_pollFD[pos].fd);
  _pollFD.erase(_pollFD.begin() + pos);
  _pollFDType.erase(_pollFDType.begin() + pos);
  --_nPollFD;
  if (pos != 0)
    --pos;
}

// Version that doesn't close the fd (for when fd was already closed)
void Webserv::removePollFD(nfds_t &pos)
{
  _pollFD.erase(_pollFD.begin() + pos);
  _pollFDType.erase(_pollFDType.begin() + pos);
  --_nPollFD;
  if (pos != 0)
    --pos;
}

void Webserv::deletePollFD(int fd)
{
  nfds_t pos = 0;
  for (; pos < _nPollFD; ++pos)
  {
    if (_pollFD[pos].fd == fd)
      break;
  }
  close(_pollFD[pos].fd);
  _pollFD.erase(_pollFD.begin() + pos);
  _pollFDType.erase(_pollFDType.begin() + pos);
  --_nPollFD;
}

int Webserv::addServer(struct conf c)
{
  Server *server = new Server();
  int fd = server->setupServer(c.name, c.port);
  if (fd == -1)
  {
    delete server;
    std::cerr << "Error: Server " << c.name << " not created" << std::endl;
    return -1;
  }
  addPollFD(fd, SERVER);
  _server[fd] = server;
  ++_nRunningServer;
  std::cerr << "Server " << server->getName()
            << " with fd " << server->getFD()
            << " listening on port " << server->getPort() << std::endl;
  return fd;
}

void Webserv::deleteServer(int fd)
{
  std::map<int, Connection *>::const_iterator it = _server[fd]->getConnection().begin();
  std::map<int, Connection *>::const_iterator endIt = _server[fd]->getConnection().end();
  int connectionFD;

  for (; it != endIt; ++it)
  {
    connectionFD = it->first;
    _connection.erase(connectionFD);
    --_nConnection;
    deletePollFD(connectionFD);
  }

  delete _server[fd];
  _server.erase(fd);
  --_nRunningServer;
}

void Webserv::restartServer(nfds_t &pos)
{
  int fd = _pollFD[pos].fd;
  struct conf c;
  c.name = _server[fd]->getName();
  c.port = _server[fd]->getPort();

  deleteServer(fd);
  deletePollFD(pos);

  if (addServer(c) == -1)
    std::cerr << "Error: server restart failed" << std::endl;
  else
  {
    std::cerr << "Server " << c.name
              << " was restarted successfully" << std::endl;
  }
}

int Webserv::addConnection(int serverFD)
{
  if (_nPollFD == MAX_FD)
  {
    // Accept and immediately close to drain the queue
    struct sockaddr_in addr;
    socklen_t addrLen = sizeof(addr);
    int tempFD = accept(serverFD, (struct sockaddr *)&addr, &addrLen);
    if (tempFD != -1)
      close(tempFD);
    DEBUG_PRINT("Error: max number of connections connected");
    return 1;
  }

  Connection *connection = new Connection();
  int connectionFD;

  connectionFD = connection->setupConnection(serverFD);

  if (connectionFD == -1)
  {
    delete connection;
    return 1;
  }

  _connection[connectionFD] = connection;
  _server[serverFD]->setConnection(connectionFD, connection);
  addPollFD(connectionFD, CONNECTION);
  ++_nConnection;

  // Debug output commented for production
  // std::cerr << "Connection accepted on port " << connection->getPort() << '\n';
  // printPoll(_nPollFD, _pollFD, _pollFDType);
  return 0;
}

void Webserv::deleteConnection(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  int serverFD = _connection[connectionFD]->getServerFD();

  // std::cerr << "Connection with socket " << connectionFD << " on port " << _connection[connectionFD]->getPort()
  //           << " deleted because";
  // if (_pollFD[pos].revents & POLLERR)
  //   std::cerr << " POLL_ERR";
  // if (_pollFD[pos].revents & POLLHUP)
  //   std::cerr << " POLL_HUP";
  // if (_pollFD[pos].revents & POLLNVAL)
  //   std::cerr << " POLLNVAL";
  // std::cerr << std::endl;

  // Clean up any pending CGI for this connection
  std::map<int, CgiState *>::iterator cgiIt = _cgiByConnection.find(connectionFD);
  if (cgiIt != _cgiByConnection.end())
  {
    cleanupCgi(cgiIt->second);
  }

  _connection.erase(connectionFD);
  --_nConnection;

  deletePollFD(pos);

  _server[serverFD]->deleteConnection(connectionFD);
}

const Config &Webserv::getConfig() const
{
  return _config;
}

void signalHandler(int signal)
{
  if (signal == SIGINT)
    throw std::runtime_error("\nWebserv closed by SIGINT signal");
  else if (signal == SIGTERM)
    throw std::runtime_error("Webserv closed by SIGTERM signal");
  else if (signal == SIGPIPE)
    throw std::runtime_error("Webserv closed by SIGPIPE signal");
}

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
    // If n <= 0, just wait for next poll - don't close the pipe
    // (poll() guarantees readiness, so -1 likely means pipe full)
  }

  // Check if all data written
  if (state->inputWritten >= state->inputData.size())
  {
    close(fd);
    state->stdinFd = -1;
    state->stdinClosed = true;
    _cgiByPipe.erase(fd);
    removePollFD(pos);
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
  // If n < 0, just wait for next poll - don't close the pipe
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

//////////////////////////
// MANDATORY MEMBER CLASS//
//////////////////////////

Webserv::Webserv(const Webserv &other)
{
  (void)other;
}

Webserv &Webserv::operator=(const Webserv &other)
{
  if (this == &other)
    return *this;
  return *this;
}
