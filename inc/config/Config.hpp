#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdlib>
#include <stdexcept>
#include <arpa/inet.h>

struct LocationConfig;
struct ServerConfig;

struct LocationConfig
{
  std::string path;
  std::string root;
  std::vector<std::string> index;
  std::set<std::string> allowedMethods;
  bool autoindex;
  bool uploadEnable;
  std::string uploadStore;
  bool cgiEnable;
  std::map<std::string, std::string> cgiPass;
  int redirectCode;
  std::string redirectUrl;
  size_t clientMaxBodySize;

  LocationConfig();
};

struct ServerConfig
{
  std::string pwd;
  std::string host;
  uint16_t port;
  std::string serverName;
  std::string root;
  std::vector<std::string> index;
  size_t clientMaxBodySize;
  std::map<int, std::string> errorPages;
  std::vector<LocationConfig> locations;

  ServerConfig();
  const LocationConfig *matchLocation(const std::string &uri) const;
};

class Config
{
public:
  Config();
  Config(const std::string &filename, const char **envp);
  Config(const Config &other);
  Config &operator=(const Config &other);
  ~Config();

  void parse(const std::string &filename, const char **envp);

  const std::vector<ServerConfig> &getServers() const;
  size_t getServerCount() const;
  const ServerConfig &getServer(size_t index) const;

private:
  std::vector<ServerConfig> _servers;
  std::string _rawContent;

  void removeComments(std::string &content);
  std::vector<std::string> tokenize(const std::string &content);
  void parseTokens(const std::vector<std::string> &tokens, const char **envp);
  void parseServerBlock(const std::vector<std::string> &tokens, size_t &pos, const char **envp);
  void parseLocationBlock(const std::vector<std::string> &tokens, size_t &pos,
                          ServerConfig &server);
  void parseDirective(const std::vector<std::string> &tokens, size_t &pos,
                      ServerConfig &server);
  void parseLocationDirective(const std::vector<std::string> &tokens, size_t &pos,
                              LocationConfig &location);

  typedef void (Config::*DirectiveHandler)(
      const std::vector<std::string> &,
      size_t &,
      ServerConfig &) const;
  std::map<std::string, DirectiveHandler> _directiveTable;

  void _handleListen(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;
  void _handleServerName(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;
  void _handleRoot(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;
  void _handleIndex(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;
  void _handleMaxBodySize(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;
  void _handleErrorPage(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const;

  typedef void (Config::*LocationDirectiveHandler)(
      const std::vector<std::string> &,
      size_t &,
      LocationConfig &) const;
  std::map<std::string, LocationDirectiveHandler> _locationDirectiveTable;

  void _handleRoot(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleIndex(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleLimitExcept(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleAutoIndex(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleUploadEnable(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleUploadStore(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleCgiEnable(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleCgiPass(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleMaxBodySize(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;
  void _handleReturn(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const;

  std::string trim(const std::string &str);
  bool isNumber(const std::string &str);
  size_t parseSize(const std::string &str) const;
};

#endif
