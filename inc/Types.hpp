#ifndef TYPES_HPP
#define TYPES_HPP

#include <sys/types.h>
#include <arpa/inet.h>

#include <vector>
#include <string>

struct CgiState
{
  pid_t pid;
  int stdinFd;
  int stdoutFd;
  int connectionFd;
  std::vector<char> inputData;
  size_t inputWritten;
  std::string outputData;
  bool stdinClosed;
  bool stdoutClosed;
  bool active;
  int pollCycles;

  CgiState();
};

typedef enum ePollFDType
{
  SERVER,
  CONNECTION,
  CGI_STDIN,
  CGI_STDOUT
} tPollFDType;

struct ServerConf
{
  std::string name;
  uint16_t port;
};

#endif
