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

  void parse(int argc, char **argv, const char **envp);
  void initServer();
  void loop();

  const Config &getConfig() const;

  void checkFDReturnedEvents();

  void receiveConnectionRequest(nfds_t &pos);
  void sendServerResponse(nfds_t &pos);

  void addPollFD(int fd, tPollFDType fdType);
  void deletePollFD(nfds_t &pos);
  void deletePollFD(int fd);
  void removePollFD(nfds_t &pos);

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
  std::map<int, size_t> _serverConfigIndex;
  nfds_t _nRunningServer;

  std::map<int, Connection *> _connection;
  nfds_t _nConnection;

  std::map<int, CgiState *> _cgiByPipe;
  std::map<int, CgiState *> _cgiByConnection;

  void handleCgiStdin(nfds_t &pos);
  void handleCgiStdinError(nfds_t &pos);
  void handleCgiStdout(nfds_t &pos);
  void finishCgi(CgiState *cgi);
  void cleanupCgi(CgiState *cgi);
  void checkCgiTimeouts();
  void timeoutCgi(CgiState *cgi);

  Webserv(const Webserv &other);
  Webserv &operator=(const Webserv &other);
};

void signalHandler(int signal);

#endif
