/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:36:27 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 14:00:13 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <arpa/inet.h>
#include <poll.h>

#include <vector>
#include <map>

#include "Server.hpp"
#include "Connection.hpp"
#include "utils.hpp"

typedef enum ePollFDType
{
  SERVER,
  CONNECTION
} tPollFDType;

class Webserv
{
public:
  Webserv();
  ~Webserv();

  void parse(int argc, char **argv);
  void initServer();
  void loop();

  void checkFDReturnedEvents();

  void receiveConnectionRequest(nfds_t &pos);
  void sendServerResponse(nfds_t &pos);

  void addPollFD(int fd, tPollFDType fdType);
  void deletePollFD(nfds_t &pos);
  void deletePollFD(int fd);

  int addServer(struct conf c);
  void deleteServer(int fd);
  void restartServer(nfds_t &pos);

  int addConnection(int serverFD);
  void deleteConnection(nfds_t &pos);

private:
  // Config _config;

  std::vector<struct pollfd> _pollFD;
  std::vector<ePollFDType> _pollFDType;
  nfds_t _nPollFD;

  std::map<int, Server *> _server;
  nfds_t _nRunningServer;

  std::map<int, Connection *> _connection;
  nfds_t _nConnection;

  Webserv(const Webserv &other);
  Webserv &operator=(const Webserv &other);
};

void signalHandler(int signal);

#endif
