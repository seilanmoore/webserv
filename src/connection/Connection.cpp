/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:36:29 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 13:58:26 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Connection.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <vector>

#include "Request.hpp"
#include "utils.hpp"

Connection::Connection()
    : _fd(-1),
      _address(),
      _serverFD(-1),
      _port(0),
      _request(NULL),
      _response(NULL)
{
}

Connection::Connection(const Connection &other)
{
  (void)other;
}

Connection &Connection::operator=(const Connection &other)
{
  if (this == &other)
    return *this;
  return *this;
}

Connection::~Connection()
{
  if (_request)
    delete _request;
  if (_response)
    delete _response;
}

int Connection::setupConnection(int serverFD)
{
  socklen_t addressLen = sizeof(_address);

  _fd = accept(serverFD, (struct sockaddr *)&_address, &addressLen);
  if (_fd == -1)
  {
    DEBUG_PRINT(errorStr(errno));
    return -1;
  }
  if (makeSocketNonBlocking(_fd) == -1)
  {
    close(_fd);
    DEBUG_PRINT(errorStr(errno));
    return -1;
  }

  _serverFD = serverFD;
  _port = ntohs(_address.sin_port);
  _request = new Request();
  _response = new Response();
  return _fd;
}

ssize_t Connection::receiveRequest()
{
  char buffer[BUFFER_SIZE + 1];
  ssize_t bytesRead;

  bytesRead = recv(_fd, buffer, BUFFER_SIZE, 0);

  if (bytesRead == -1)
  {
    DEBUG_VAR(errorStr(errno), getPort());
    return -1;
  }

  if (bytesRead == 0)
  {
    std::ostringstream oss;
    oss << "\nConnection on port " << getPort() << " disconnected";
    DEBUG_PRINT(oss.str());
    return -1;
  }

  buffer[bytesRead] = '\0';

  tRecvStatus recvStatus = _request->getRecvStatus();

  if (recvStatus == HEADER)
    _request->setHeader(buffer);
  else if (recvStatus == BODY)
    _request->setBody(buffer, bytesRead);

  recvStatus = _request->getRecvStatus();
  if (recvStatus == DONE)
  {
    std::ostringstream oss;
    oss << "\nRequest from connection on port " << _port << ":\n"
        << "///////////////////////////////////////////////////\n"
        << _request->getHeader()
        << "\n///////////////////////////////////////////////////";
    DEBUG_PRINT(oss.str());
    return 0;
  }

  return bytesRead;
}

void Connection::generateResponse(const ServerConfig &serverConfig)
{
  _response->generateResponse(*_request, serverConfig);
}

ssize_t Connection::sendServerResponse()
{
  return _response->sendResponse(_fd);
}

void Connection::resetRequestResponse()
{
  delete _request;
  delete _response;
  _request = new Request();
  _response = new Response();
}

uint16_t Connection::getPort() const
{
  return _port;
}

int Connection::getServerFD() const
{
  return _serverFD;
}

bool Connection::shouldKeepAlive() const
{
  if (_request == NULL)
    return false;
  return _request->isKeepAlive();
}

bool Connection::hasPendingCgi() const
{
  if (_response == NULL)
    return false;
  return _response->hasPendingCgi();
}
