/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:55:34 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 14:03:39 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Webserv.hpp"

#include <unistd.h>
#include <poll.h>
#include <sys/types.h>
#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstring>

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>

#include "Server.hpp"
#include "Connection.hpp"
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
  if (argc == 1)
    ; // webserv.parseConfig(DEFAULT_CONFIG_FILE_PATH);
  else if (argc == 2)
    (void)argv; // webserv.parseConfig(argv[1]);
  else
    throw std::runtime_error("Usage: ./webserv [configuration file]");
}

struct conf
{
  std::string name;
  uint16_t port;
};

void Webserv::initServer()
{
  // uint16_t nServer = _config.getNbServer();
  nfds_t nServer = 5;
  conf c[5] = {
      {"name0", 8080},
      {"name1", 8081},
      {"name2", 8082},
      {"name3", 8083},
      {"name4", 8084},
  };

  for (nfds_t i = 0; i < nServer; ++i)
    addServer(c[i]);

  _nPollFD = static_cast<nfds_t>(_nRunningServer);
}

void Webserv::loop()
{
  signal(SIGINT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGPIPE, signalHandler);
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
      std::cerr << "Debug: timeout. Connected connections "
                << _nPollFD - _nRunningServer << '\n';
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
      // std::cerr << "Error: poll error flag on socket "
      //           << _pollFD[pos].fd << std::endl;
      if (_pollFDType[pos] == SERVER)
        restartServer(pos);
      else if (_pollFDType[pos] == CONNECTION)
        deleteConnection(pos);
    }
    else if (_pollFD[pos].revents & POLLIN)
    {
      if (_pollFDType[pos] == SERVER)
        addConnection(_pollFD[pos].fd);
      else if (_pollFDType[pos] == CONNECTION)
        receiveConnectionRequest(pos);
    }
    else if (_pollFD[pos].revents & POLLOUT)
      sendServerResponse(pos);

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

    connection->generateResponse();
    _pollFD[pos].events = POLLOUT;
  }
}

void Webserv::sendServerResponse(nfds_t &pos)
{
  int connectionFD = _pollFD[pos].fd;
  ssize_t sendStatus = _connection[connectionFD]->sendServerResponse();
  if (sendStatus == -1)
  {
    DEBUG_VAR(errorStr(errno), _pollFD[pos].fd);
    deleteConnection(pos);
  }
  else if (sendStatus == 0)
  {
    std::cerr << "Response: " << _connection[connectionFD]->getResponse();

    _pollFD[pos].events = POLLIN;
    _connection[connectionFD]->resetRequestResponse();
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

  std::cerr << "Connection accepted on port " << connection->getPort() << '\n';
  printPoll(_nPollFD, _pollFD, _pollFDType);
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

  _connection.erase(connectionFD);
  --_nConnection;

  deletePollFD(pos);

  _server[serverFD]->deleteConnection(connectionFD);
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
