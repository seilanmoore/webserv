/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:21 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:54 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"

#include <arpa/inet.h>  // htons()
#include <netinet/in.h> // struct sockaddr_in
#include <poll.h>
#include <sys/socket.h> // socket(), setsockopt()
#include <unistd.h>     // close(), dup()
#include <cerrno>
#include <cstring>

#include <iostream>
#include <stdexcept>

#include "utils.hpp"

// Server::Server(const sDirectives *httpDirective, const sServer &config)
//     : _genericDir(httpDirective),
//       _config(config),
//     :  _address(),
//       _fd(),
//       _name(httpDirective->serverName),
//       _port()
// {
// }

Server::Server()
    : _name(""), _port()
{
}

Server::~Server()
{
  std::map<int, Connection *>::iterator it = _connection.begin();

  for (; it != _connection.end(); ++it)
    delete it->second;
}

int Server::setupServer(const std::string &name, uint16_t port)
{
  _name = name;
  _port = port;
  int opt = 1;

  if ((_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0)) == -1)
  {
    std::cerr << errorStr(errno) << '\n';
    return -1;
  }

  if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
  {
    std::cerr << errorStr(errno) << '\n';
    return -1;
  }

  // if (makeSocketNonBlocking(_fd) == -1)
  // {
  //   std::cerr << errorStr(errno) << '\n';
  //   return -1;
  // }

  _address.sin_family = AF_INET;
  _address.sin_addr.s_addr = INADDR_ANY;
  _address.sin_port = htons(_port);

  if (bind(_fd, (struct sockaddr *)&_address, sizeof(_address)) < 0)
  {
    std::cerr << errorStr(errno) << '\n';
    return -1;
  }

  if (listen(_fd, 1024) < 0)
  {
    std::cerr << errorStr(errno) << '\n';
    return -1;
  }

  std::cerr << "Server " << _name << " created\n";
  return _fd;
}

void Server::deleteConnection(int fd)
{
  delete _connection[fd];
  _connection.erase(fd);
  --_nConnection;
}

t_sockaddr_in Server::getAddress() const
{
  return _address;
}

int Server::getFD() const
{
  return _fd;
}

const std::string &Server::getName() const
{
  return _name;
}
uint16_t Server::getPort() const
{
  return _port;
}

const std::map<int, Connection *> &Server::getConnection() const
{
  return _connection;
}

nfds_t Server::getNConnection() const
{
  return _nConnection;
}

void Server::setConnection(int fd, Connection *connection)
{
  _connection[fd] = connection;
  ++_nConnection;
}

// MANDATORY USELESS TRASH

// unusable overload
Server::Server(const Server &other)
{
  (void)other;
}

// unusable overload
Server &Server::operator=(const Server &other)
{
  (void)other;
  return *this;
}
