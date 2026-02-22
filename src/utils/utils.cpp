#include "utils.hpp"

#include <fcntl.h>
#include <cstring>

#include <sstream> //itoa

#include <iostream>
#include <stdexcept>

const std::string errorStr(int code)
{
  std::string str("Error: ");

  str.append(strerror(code));
  return str;
}

int makeSocketNonBlocking(int socketFD)
{
  if (fcntl(socketFD, F_SETFL, O_NONBLOCK) == -1)
    return -1;
  return 0;
}
