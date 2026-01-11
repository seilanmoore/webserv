/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 13:45:07 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/10 19:31:52 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
#include <arpa/inet.h> // uint16_t

// Forward declarations
struct LocationConfig;
struct ServerConfig;

// ============================================================================
// LocationConfig: Configuration for a specific route/location
// ============================================================================
struct LocationConfig
{
  std::string path;                           // Location path (e.g., "/", "/upload")
  std::string root;                           // Root directory for this location
  std::vector<std::string> index;             // Default index files
  std::set<std::string> allowedMethods;       // Allowed HTTP methods
  bool autoindex;                             // Directory listing enabled
  bool uploadEnable;                          // File upload enabled
  std::string uploadStore;                    // Upload storage directory
  bool cgiEnable;                             // CGI execution enabled
  std::map<std::string, std::string> cgiPass; // Extension -> interpreter mapping
  int redirectCode;                           // Redirect status code (0 = no redirect)
  std::string redirectUrl;                    // Redirect target URL
  size_t clientMaxBodySize;                   // Max request body size (0 = use server default)

  LocationConfig();
};

// ============================================================================
// ServerConfig: Configuration for a virtual server
// ============================================================================
struct ServerConfig
{
  std::string host;                      // Listen address (default: 0.0.0.0)
  uint16_t port;                         // Listen port
  std::string serverName;                // Server name (optional)
  std::string root;                      // Default root directory
  std::vector<std::string> index;        // Default index files
  size_t clientMaxBodySize;              // Max request body size in bytes
  std::map<int, std::string> errorPages; // Error code -> error page path
  std::vector<LocationConfig> locations; // Location blocks

  ServerConfig();
  const LocationConfig *matchLocation(const std::string &uri) const;
};

// ============================================================================
// Config: Main configuration class - parses and stores all configuration
// ============================================================================
class Config
{
public:
  Config();
  Config(const std::string &filename);
  Config(const Config &other);
  Config &operator=(const Config &other);
  ~Config();

  // Parse configuration file
  void parse(const std::string &filename);

  // Getters
  const std::vector<ServerConfig> &getServers() const;
  size_t getServerCount() const;
  const ServerConfig &getServer(size_t index) const;

private:
  std::vector<ServerConfig> _servers;
  std::string _rawContent;

  // Parsing helpers
  void removeComments(std::string &content);
  std::vector<std::string> tokenize(const std::string &content);
  void parseTokens(const std::vector<std::string> &tokens);
  void parseServerBlock(const std::vector<std::string> &tokens, size_t &pos);
  void parseLocationBlock(const std::vector<std::string> &tokens, size_t &pos,
                          ServerConfig &server);
  void parseDirective(const std::vector<std::string> &tokens, size_t &pos,
                      ServerConfig &server);
  void parseLocationDirective(const std::vector<std::string> &tokens, size_t &pos,
                              LocationConfig &location);

  // Utility functions
  std::string trim(const std::string &str);
  bool isNumber(const std::string &str);
  size_t parseSize(const std::string &str);
};

#endif
