#include "Config.hpp"

// ============================================================================
// LocationConfig Implementation
// ============================================================================

LocationConfig::LocationConfig()
    : path("/"),
      root(),
      index(),
      allowedMethods(),
      autoindex(false),
      uploadEnable(false),
      uploadStore(),
      cgiEnable(false),
      cgiPass(),
      redirectCode(0),
      redirectUrl(),
      clientMaxBodySize(0)
{
  allowedMethods.insert("GET");
  allowedMethods.insert("POST");
  allowedMethods.insert("DELETE");
}

// ============================================================================
// ServerConfig Implementation
// ============================================================================

ServerConfig::ServerConfig()
    : host("0.0.0.0"),
      port(8080),
      serverName(),
      root("./docs/html"),
      index(),
      clientMaxBodySize(1048576),
      errorPages(),
      locations()
{
  index.push_back("index.html");
}

const LocationConfig *ServerConfig::matchLocation(const std::string &uri) const
{
  const LocationConfig *bestMatch = NULL;
  size_t bestMatchLen = 0;

  for (size_t i = 0; i < locations.size(); ++i)
  {
    const std::string &locPath = locations[i].path;

    if (uri.compare(0, locPath.length(), locPath) == 0)
    {
      if (locPath.length() > bestMatchLen)
      {
        if (locPath == "/" ||
            uri.length() == locPath.length() ||
            uri[locPath.length()] == '/')
        {
          bestMatch = &locations[i];
          bestMatchLen = locPath.length();
        }
      }
    }
  }

  return bestMatch;
}

// ============================================================================
// Config Implementation
// ============================================================================

Config::Config()
    : _servers(),
      _rawContent()
{
  _directiveTable["listen"] = &Config::_handleListen;
  _directiveTable["server_name"] = &Config::_handleServerName;
  _directiveTable["root"] = &Config::_handleRoot;
  _directiveTable["index"] = &Config::_handleIndex;
  _directiveTable["client_max_body_size"] = &Config::_handleMaxBodySize;
  _directiveTable["error_page"] = &Config::_handleErrorPage;

  _locationDirectiveTable["root"] = &Config::_handleRoot;
  _locationDirectiveTable["index"] = &Config::_handleIndex;
  _locationDirectiveTable["limit_except"] = &Config::_handleLimitExcept;
  _locationDirectiveTable["autoindex"] = &Config::_handleAutoIndex;
  _locationDirectiveTable["upload_enable"] = &Config::_handleUploadEnable;
  _locationDirectiveTable["upload_store"] = &Config::_handleUploadStore;
  _locationDirectiveTable["cgi_enable"] = &Config::_handleCgiEnable;
  _locationDirectiveTable["cgi_pass"] = &Config::_handleCgiPass;
  _locationDirectiveTable["client_max_body_size"] = &Config::_handleMaxBodySize;
  _locationDirectiveTable["return"] = &Config::_handleReturn;
}

Config::Config(const std::string &filename, const char **envp)
    : _servers(),
      _rawContent()
{
  _directiveTable["listen"] = &Config::_handleListen;
  _directiveTable["server_name"] = &Config::_handleServerName;
  _directiveTable["root"] = &Config::_handleRoot;
  _directiveTable["index"] = &Config::_handleIndex;
  _directiveTable["client_max_body_size"] = &Config::_handleMaxBodySize;
  _directiveTable["error_page"] = &Config::_handleErrorPage;

  _locationDirectiveTable["root"] = &Config::_handleRoot;
  _locationDirectiveTable["index"] = &Config::_handleIndex;
  _locationDirectiveTable["limit_except"] = &Config::_handleLimitExcept;
  _locationDirectiveTable["autoindex"] = &Config::_handleAutoIndex;
  _locationDirectiveTable["upload_enable"] = &Config::_handleUploadEnable;
  _locationDirectiveTable["upload_store"] = &Config::_handleUploadStore;
  _locationDirectiveTable["cgi_enable"] = &Config::_handleCgiEnable;
  _locationDirectiveTable["cgi_pass"] = &Config::_handleCgiPass;
  _locationDirectiveTable["client_max_body_size"] = &Config::_handleMaxBodySize;
  _locationDirectiveTable["return"] = &Config::_handleReturn;

  parse(filename, envp);
}

Config::Config(const Config &other)
    : _servers(other._servers),
      _rawContent(other._rawContent)
{
}

Config &Config::operator=(const Config &other)
{
  if (this != &other)
  {
    _servers = other._servers;
    _rawContent = other._rawContent;
  }
  return *this;
}

Config::~Config()
{
}

// ============================================================================
// Main Parse Function
// ============================================================================

void Config::parse(const std::string &filename, const char **envp)
{
  std::ifstream file(filename.c_str());
  if (!file.is_open())
    throw std::runtime_error("Config: Cannot open file: " + filename);

  std::stringstream buffer;
  buffer << file.rdbuf();
  _rawContent = buffer.str();
  file.close();

  removeComments(_rawContent);
  std::vector<std::string> tokens = tokenize(_rawContent);

  parseTokens(tokens, envp);

  if (_servers.empty())
    throw std::runtime_error("Config: No server blocks found");
}

// ============================================================================
// Parsing Helpers
// ============================================================================

void Config::removeComments(std::string &content)
{
  std::string result;
  std::istringstream iss(content);
  std::string line;

  while (std::getline(iss, line))
  {
    size_t commentPos = line.find('#');
    if (commentPos != std::string::npos)
      line = line.substr(0, commentPos);
    result += line + "\n";
  }
  content = result;
}

std::vector<std::string> Config::tokenize(const std::string &content)
{
  std::vector<std::string> tokens;
  std::string current;

  for (size_t i = 0; i < content.length(); ++i)
  {
    char c = content[i];

    if (c == '{' || c == '}' || c == ';')
    {
      if (!current.empty())
      {
        tokens.push_back(current);
        current.clear();
      }
      tokens.push_back(std::string(1, c));
    }
    else if (std::isspace(c))
    {
      if (!current.empty())
      {
        tokens.push_back(current);
        current.clear();
      }
    }
    else
    {
      current += c;
    }
  }

  if (!current.empty())
    tokens.push_back(current);

  return tokens;
}

void Config::parseTokens(const std::vector<std::string> &tokens, const char **envp)
{
  size_t pos = 0;

  while (pos < tokens.size())
  {
    if (tokens[pos] == "http")
    {
      ++pos;
      if (pos >= tokens.size() || tokens[pos] != "{")
        throw std::runtime_error("Config: Expected '{' after 'http'");
      ++pos;

      while (pos < tokens.size() && tokens[pos] != "}")
      {
        if (tokens[pos] == "server")
        {
          parseServerBlock(tokens, pos, envp);
        }
        else
        {
          ++pos;
        }
      }

      if (pos < tokens.size())
        ++pos;
    }
    else if (tokens[pos] == "server")
    {
      parseServerBlock(tokens, pos, envp);
    }
    else
    {
      ++pos;
    }
  }
}

void Config::parseServerBlock(const std::vector<std::string> &tokens, size_t &pos, const char **envp)
{
  ServerConfig server;

  ++pos;
  if (pos >= tokens.size() || tokens[pos] != "{")
    throw std::runtime_error("Config: Expected '{' after 'server'");
  ++pos;

  while (pos < tokens.size() && tokens[pos] != "}")
  {
    if (tokens[pos] == "location")
    {
      parseLocationBlock(tokens, pos, server);
    }
    else
    {
      parseDirective(tokens, pos, server);
    }
  }

  if (pos < tokens.size())
    ++pos;

  std::string var;
  for (size_t i = 0; envp[i]; ++i)
  {
    var = std::string(envp[i]);
    if (var.substr(0, 4).compare("PWD=") == 0)
    {
      server.pwd = var.substr(var.find_first_of('=') + 1);
    }
  }

  _servers.push_back(server);
}

void Config::parseLocationBlock(const std::vector<std::string> &tokens, size_t &pos,
                                ServerConfig &server)
{
  LocationConfig location;
  location.root = server.root;
  location.index = server.index;

  ++pos;
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected location path");

  location.path = tokens[pos];
  ++pos;

  if (pos >= tokens.size() || tokens[pos] != "{")
    throw std::runtime_error("Config: Expected '{' after location path");
  ++pos;

  while (pos < tokens.size() && tokens[pos] != "}")
  {
    parseLocationDirective(tokens, pos, location);
  }

  if (pos < tokens.size())
    ++pos;
  server.locations.push_back(location);
}

void Config::parseDirective(const std::vector<std::string> &tokens, size_t &pos,
                            ServerConfig &server)
{
  std::string directive = tokens[pos];
  ++pos;

  std::map<std::string, DirectiveHandler>::iterator it = _directiveTable.find(directive);

  if (it != _directiveTable.end())
  {
    DirectiveHandler handler = it->second;
    (this->*handler)(tokens, pos, server);
  }
  else
  {
    while (pos < tokens.size() && tokens[pos] != ";" && tokens[pos] != "}")
      ++pos;
    if (pos < tokens.size() && tokens[pos] == ";")
      ++pos;
  }
}

// ============================================================================
// DirectiveHandlers
// ============================================================================

void Config::_handleListen(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'listen'");

  std::string value = tokens[pos];
  ++pos;

  size_t colonPos = value.find(':');
  if (colonPos != std::string::npos)
  {
    server.host = value.substr(0, colonPos);
    server.port = static_cast<uint16_t>(std::atoi(value.substr(colonPos + 1).c_str()));
  }
  else
  {
    server.port = static_cast<uint16_t>(std::atoi(value.c_str()));
  }

  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleServerName(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'server_name'");
  server.serverName = tokens[pos];
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleRoot(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'root'");
  server.root = tokens[pos];
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleIndex(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  server.index.clear();
  while (pos < tokens.size() && tokens[pos] != ";" && tokens[pos] != "}")
  {
    server.index.push_back(tokens[pos]);
    ++pos;
  }
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleMaxBodySize(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'client_max_body_size'");
  server.clientMaxBodySize = parseSize(tokens[pos]);
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleErrorPage(const std::vector<std::string> &tokens, size_t &pos, ServerConfig &server) const
{
  if (pos + 1 >= tokens.size())
    throw std::runtime_error("Config: Expected code and path after 'error_page'");
  int code = std::atoi(tokens[pos].c_str());
  ++pos;
  std::string path = tokens[pos];
  ++pos;
  server.errorPages[code] = path;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::parseLocationDirective(const std::vector<std::string> &tokens, size_t &pos,
                                    LocationConfig &location)
{
  std::string directive = tokens[pos];
  ++pos;

  std::map<std::string, LocationDirectiveHandler>::iterator it = _locationDirectiveTable.find(directive);

  if (it != _locationDirectiveTable.end())
  {
    LocationDirectiveHandler handler = it->second;
    (this->*handler)(tokens, pos, location);
  }
  else
  {
    while (pos < tokens.size() && tokens[pos] != ";" && tokens[pos] != "}")
      ++pos;
    if (pos < tokens.size() && tokens[pos] == ";")
      ++pos;
  }
}

// ============================================================================
// LocationDirectiveHandlers
// ============================================================================

void Config::_handleRoot(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'root'");
  location.root = tokens[pos];
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleIndex(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  location.index.clear();
  while (pos < tokens.size() && tokens[pos] != ";" && tokens[pos] != "}")
  {
    location.index.push_back(tokens[pos]);
    ++pos;
  }
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleLimitExcept(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  location.allowedMethods.clear();
  while (pos < tokens.size() && tokens[pos] != ";" && tokens[pos] != "}")
  {
    location.allowedMethods.insert(tokens[pos]);
    ++pos;
  }
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleAutoIndex(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'autoindex'");
  location.autoindex = (tokens[pos] == "on");
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleUploadEnable(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'upload_enable'");
  location.uploadEnable = (tokens[pos] == "on");
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleUploadStore(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'upload_store'");
  location.uploadStore = tokens[pos];
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleCgiEnable(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'cgi_enable'");
  location.cgiEnable = (tokens[pos] == "on");
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleCgiPass(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos + 1 >= tokens.size())
    throw std::runtime_error("Config: Expected extension and interpreter after 'cgi_pass'");
  std::string extension = tokens[pos];
  ++pos;
  std::string interpreter = tokens[pos];
  ++pos;
  location.cgiPass[extension] = interpreter;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleMaxBodySize(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos >= tokens.size())
    throw std::runtime_error("Config: Expected value after 'client_max_body_size'");
  location.clientMaxBodySize = parseSize(tokens[pos]);
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

void Config::_handleReturn(const std::vector<std::string> &tokens, size_t &pos, LocationConfig &location) const
{
  if (pos + 1 >= tokens.size())
    throw std::runtime_error("Config: Expected code and URL after 'return'");
  location.redirectCode = std::atoi(tokens[pos].c_str());
  ++pos;
  location.redirectUrl = tokens[pos];
  ++pos;
  if (pos < tokens.size() && tokens[pos] == ";")
    ++pos;
}

// ============================================================================
// Utility Functions
// ============================================================================

std::string Config::trim(const std::string &str)
{
  size_t start = str.find_first_not_of(" \t\n\r");
  if (start == std::string::npos)
    return "";
  size_t end = str.find_last_not_of(" \t\n\r");
  return str.substr(start, end - start + 1);
}

bool Config::isNumber(const std::string &str)
{
  for (size_t i = 0; i < str.length(); ++i)
  {
    if (!std::isdigit(str[i]))
      return false;
  }
  return !str.empty();
}

size_t Config::parseSize(const std::string &str) const
{
  size_t len = str.length();
  if (len == 0)
    return 0;

  char suffix = str[len - 1];
  std::string numPart = str;
  size_t multiplier = 1;

  if (!std::isdigit(suffix))
  {
    numPart = str.substr(0, len - 1);
    switch (suffix)
    {
    case 'k':
    case 'K':
      multiplier = 1024;
      break;
    case 'm':
    case 'M':
      multiplier = 1024 * 1024;
      break;
    case 'g':
    case 'G':
      multiplier = 1024 * 1024 * 1024;
      break;
    default:
      break;
    }
  }

  return static_cast<size_t>(std::atol(numPart.c_str())) * multiplier;
}

// ============================================================================
// Getters
// ============================================================================

const std::vector<ServerConfig> &Config::getServers() const
{
  return _servers;
}

size_t Config::getServerCount() const
{
  return _servers.size();
}

const ServerConfig &Config::getServer(size_t index) const
{
  if (index >= _servers.size())
    throw std::out_of_range("Config: Server index out of range");
  return _servers[index];
}
