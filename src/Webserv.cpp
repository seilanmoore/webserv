/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:55:34 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:57:35 by smoore-a         ###   ########.fr       */
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

// ============================================================================
// Debug Helpers
// ============================================================================

static void printPoll(nfds_t nPollFD, const std::vector<struct pollfd> &pollFD, const std::vector<ePollFDType> &type)
{
  std::cerr << "POLL FD\n";
  std::cerr << "-------\n";
  for (nfds_t i = 0; i < nPollFD; ++i)
  {
    std::cerr << (type[i] == SERVER ? "Server" : "Connection") << ": " << pollFD[i].fd << '\n';
  }
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

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

// ============================================================================
// Initialization
// ============================================================================

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

void Webserv::initServer()
{
  const std::vector<ServerConfig> &servers = _config.getServers();

  for (size_t i = 0; i < servers.size(); ++i)
  {
    ServerConf c;
    if (servers[i].serverName.empty())
    {
      std::ostringstream ss;
      ss << i;
      c.name = "server" + ss.str();
    }
    else
      c.name = servers[i].serverName;
    c.port = servers[i].port;
    int fd = addServer(c);
    if (fd != -1)
      _serverConfigIndex[fd] = i;
  }

  _nPollFD = static_cast<nfds_t>(_nRunningServer);
}

// ============================================================================
// Main Event Loop
// ============================================================================

void Webserv::loop()
{
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  // SIGPIPE is ignored in main() to prevent crash on broken pipe
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
      // Timeout occurred, no events - check for CGI timeouts
      checkCgiTimeouts();
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
      else if (_pollFDType[pos] == CGI_STDIN)
      {
        // CGI stdin pipe error - close it and mark as done
        handleCgiStdinError(pos);
      }
      else if (_pollFDType[pos] == CGI_STDOUT)
      {
        // CGI stdout pipe error/close - try to read remaining data
        handleCgiStdout(pos);
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

// ============================================================================
// Connection Request/Response Handling
// ============================================================================

void Webserv::receiveConnectionRequest(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  Connection *connection = _connection[connectionFD];

  // If there's a pending CGI, we keep POLLIN to detect client disconnect
  // but we don't actually read any data - just ignore POLLIN events
  if (connection->hasPendingCgi())
  {
    return;
  }

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
      CgiState &cgi = resp->getCgiInfo();

      // Create CGI state
      CgiState *state = new CgiState();
      state->pid = cgi.pid;
      state->stdinFd = cgi.stdinFd;
      state->stdoutFd = cgi.stdoutFd;
      state->connectionFd = connectionFD;
      state->inputData.swap(cgi.inputData); // Move data without copying
      state->inputWritten = cgi.inputWritten;
      state->outputData.swap(cgi.outputData); // Move data without copying
      state->stdinClosed = cgi.stdinClosed;
      state->stdoutClosed = false;
      state->active = true;
      state->pollCycles = 0;

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
      // Keep POLLIN to detect client disconnection during CGI processing
      _pollFD[pos].events = POLLIN;
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
    // Check if client requested Connection: close
    if (!conn->shouldKeepAlive())
    {
      deleteConnection(pos);
      return;
    }

    _pollFD[pos].events = POLLIN;
    conn->resetRequestResponse();
  }
}

// ============================================================================
// Poll FD Management
// ============================================================================

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

// ============================================================================
// Server Management
// ============================================================================

int Webserv::addServer(const ServerConf &c)
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
  ServerConf c;
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

// ============================================================================
// Connection Management
// ============================================================================

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

  return 0;
}

void Webserv::deleteConnection(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  int serverFD = _connection[connectionFD]->getServerFD();

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

// ============================================================================
// Getters
// ============================================================================

const Config &Webserv::getConfig() const
{
  return _config;
}

// ============================================================================
// Signal Handler
// ============================================================================

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
// Mandatory Class Members (Orthodox Canonical Form)
// ============================================================================

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
