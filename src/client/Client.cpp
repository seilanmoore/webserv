/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/18 14:36:29 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 11:51:44 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <sstream>
#include <fstream>
#include <vector>

#include "Request.hpp"
#include "utils.hpp"

Client::Client()
    : _fd(-1),
      _address(),
      _serverFD(-1),
      _port(0),
      _request(NULL),
      _response(NULL)
{
}

Client::Client(const Client &other)
{
  (void)other;
}

Client &Client::operator=(const Client &other)
{
  if (this == &other)
    return *this;
  return *this;
}

Client::~Client()
{
  if (_request)
    delete _request;
  if (_response)
    delete _response;
}

int Client::setupClient(int serverFD)
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

ssize_t Client::receiveRequest()
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
    std::cerr << "Client socket " << _fd << " on port " << getPort() << " disconnected" << std::endl;
    return -1;
  }

  buffer[bytesRead] = '\0';

  // std::cerr << "Read: " << buffer << '\n';
  // std::cerr << "Bytes: " << bytesRead << '\n';

  tRecvStatus recvStatus = _request->getRecvStatus();

  if (recvStatus == HEADER)
    _request->setHeader(buffer, bytesRead);
  else if (recvStatus == BODY)
    _request->setBody(buffer, bytesRead);

  recvStatus = _request->getRecvStatus();
  if (recvStatus == DONE)
  {
    std::cerr << "Request from client socket " << _fd << " on port " << _port << ":\n"
              << _request->getHeader() << '\n';
    return 0;
  }

  return bytesRead;
}

void Client::generateResponse()
{
  _response->generateResponse(*_request);
}

ssize_t Client::sendServerResponse()
{
  return _response->sendResponse(_fd);
}

void Client::resetRequestResponse()
{
  delete _request;
  delete _response;
  _request = new Request();
  _response = new Response();
}

uint16_t Client::getPort() const
{
  return _port;
}

int Client::getServerFD() const
{
  return _serverFD;
}

const std::string &Client::getResponse() const
{
  return _response->getResponse();
}
