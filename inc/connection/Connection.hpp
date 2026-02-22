#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Connection
{
public:
  Connection();
  Connection(const Connection &other);
  Connection &operator=(const Connection &other);
  ~Connection();

  int setupConnection(int serverFD);

  ssize_t receiveRequest();

  void generateResponse(const ServerConfig &serverConfig);
  ssize_t sendServerResponse();

  void resetRequestResponse();

  uint16_t getPort() const;
  int getServerFD() const;

  bool shouldKeepAlive() const;

  bool hasPendingCgi() const;
  Response *getResponsePtr();

private:
  int _fd;
  struct sockaddr_in _address;
  int _serverFD;
  uint16_t _port;

  Request *_request;
  Response *_response;
};

#endif
