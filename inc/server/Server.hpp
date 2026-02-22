#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>  // uint16_t
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket()
#include <poll.h>       // nfds_t

#include <map>
#include <string>

#include "Connection.hpp"

typedef struct sockaddr_in t_sockaddr_in;

class Server
{
public:
  Server();
  ~Server();

  int setupServer(const std::string &name, uint16_t port);

  void deleteConnection(int fd);

  int getFD() const;
  t_sockaddr_in getAddress() const;
  const std::string &getName() const;
  uint16_t getPort() const;
  const std::map<int, Connection *> &getConnection() const;
  nfds_t getNConnection() const;

  void setConnection(int fd, Connection *connection);

private:
  t_sockaddr_in _address;
  int _fd;
  std::string _name;
  uint16_t _port;

  std::map<int, Connection *> _connection;
  nfds_t _nConnection;

  Server(const Server &other);
  Server &operator=(const Server &other);
};

#endif
