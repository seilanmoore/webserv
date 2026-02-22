#ifndef UTILS_HPP
#define UTILS_HPP

#include <unistd.h>

#include <iostream>
#include <string>

#ifdef DEBUG
#include <iostream>
#include <cassert>
#define DEBUG_PRINT(msg)                                                 \
  do                                                                     \
  {                                                                      \
    std::cerr << msg << std::endl;                                       \
    std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << std::endl; \
  } while (0)
#define DEBUG_ASSERT(cond) assert(cond)
#define DEBUG_VAR(msg, var)                                              \
  do                                                                     \
  {                                                                      \
    std::cerr << msg << ": " << #var << " = " << (var) << std::endl;     \
    std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << std::endl; \
  } while (0)
#else
#define DEBUG_PRINT(msg)           \
  do                               \
  {                                \
    std::cerr << msg << std::endl; \
  } while (0)
#define DEBUG_ASSERT(cond) ((void)0)
#define DEBUG_VAR(msg, var)                                          \
  do                                                                 \
  {                                                                  \
    std::cerr << msg << ": " << #var << " = " << (var) << std::endl; \
  } while (0)
#endif

const std::string errorStr(int code);

int makeSocketNonBlocking(int socketFD);

#endif
