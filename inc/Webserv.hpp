/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/09 12:36:27 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:29 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include <poll.h>

#include <vector>
#include <map>

#include "Types.hpp"
#include "Constants.hpp"
#include "Config.hpp"
#include "Server.hpp"
#include "Connection.hpp"

class Webserv
{
public:
  Webserv();
  ~Webserv();

  void parse(int argc, char **argv);
  void initServer();
  void loop();

  const Config &getConfig() const;

  void checkFDReturnedEvents();

  void receiveConnectionRequest(nfds_t &pos);
  void sendServerResponse(nfds_t &pos);

  void addPollFD(int fd, tPollFDType fdType);
  void deletePollFD(nfds_t &pos);
  void deletePollFD(int fd);
  void removePollFD(nfds_t &pos); // Remove from poll without closing fd

  int addServer(const ServerConf &c);
  void deleteServer(int fd);
  void restartServer(nfds_t &pos);

  int addConnection(int serverFD);
  void deleteConnection(nfds_t &pos);

private:
  Config _config;

  std::vector<struct pollfd> _pollFD;
  std::vector<ePollFDType> _pollFDType;
  nfds_t _nPollFD;

  std::map<int, Server *> _server;
  std::map<int, size_t> _serverConfigIndex; // Maps server FD to config index
  nfds_t _nRunningServer;

  std::map<int, Connection *> _connection;
  nfds_t _nConnection;

  // CGI state management
  std::map<int, CgiState *> _cgiByPipe;       // Maps pipe FD to CGI state
  std::map<int, CgiState *> _cgiByConnection; // Maps connection FD to CGI state

  void handleCgiStdin(nfds_t &pos);
  void handleCgiStdinError(nfds_t &pos);
  void handleCgiStdout(nfds_t &pos);
  void finishCgi(CgiState *cgi);
  void cleanupCgi(CgiState *cgi);

  Webserv(const Webserv &other);
  Webserv &operator=(const Webserv &other);
};

void signalHandler(int signal);

#endif
