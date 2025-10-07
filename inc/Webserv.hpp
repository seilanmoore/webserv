/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:36:27 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/02 20:01:14 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <arpa/inet.h>
#include <poll.h>

#include <vector>
#include <map>

#include "Server.hpp"
#include "Client.hpp"
#include "utils.hpp"

typedef enum ePollFDType
{
  SERVER,
  CLIENT
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

  void receiveClientRequest(nfds_t &pos);
  void sendServerResponse(nfds_t &pos);

  void addPollFD(int fd, tPollFDType fdType);
  void deletePollFD(nfds_t &pos);
  void deletePollFD(int fd);

  int addServer(struct conf c);
  void deleteServer(int fd);
  void restartServer(nfds_t &pos);

  int addClient(int serverFD);
  void deleteClient(nfds_t &pos);

private:
  // Config _config;

  std::vector<struct pollfd> _pollFD;
  std::vector<ePollFDType> _pollFDType;
  nfds_t _nPollFD;

  std::map<int, Server *> _server;
  nfds_t _nRunningServer;

  std::map<int, Client *> _client;
  nfds_t _nClient;

  Webserv(const Webserv &other);
  Webserv &operator=(const Webserv &other);
};

void signalHandler(int signal);

#endif
