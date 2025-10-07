/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:06:57 by smoore-a          #+#    #+#             */
/*   Updated: 2025/09/23 13:56:42 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>  // uint16_t
#include <netinet/in.h> // struct sockaddr_in
#include <sys/socket.h> // socket()
#include <poll.h>       // nfds_t

#include <map>
#include <string>

// #include "Config.hpp"
#include "Client.hpp"

typedef struct sockaddr_in t_sockaddr_in;

class Server
{
public:
  Server();
  ~Server();

  int setupServer(const std::string &name, uint16_t port);

  void deleteClient(int fd);

  int getFD() const;
  t_sockaddr_in getAddress() const;
  const std::string &getName() const;
  uint16_t getPort() const;
  const std::map<int, Client *> &getClient() const;
  nfds_t getNClient() const;

  void setFD(int fd);
  void setAddress(const t_sockaddr_in &address);
  void setClient(int fd, Client *client);

private:
  // const sDirectives *_genericDir;
  // const sServer &_config;

  t_sockaddr_in _address;
  int _fd;
  std::string _name;
  uint16_t _port;

  std::map<int, Client *> _client;
  nfds_t _nClient;

  // MANDATORY CLASS MEMBERS
  Server(const Server &other);
  Server &operator=(const Server &other);
};

#endif
