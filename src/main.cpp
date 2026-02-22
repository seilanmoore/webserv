#include <exception>
#include <iostream>
#include <stdexcept>
#include <signal.h>

#include "Webserv.hpp"

int main(int argc, char **argv, char **envp)
{
  signal(SIGPIPE, SIG_IGN);

  Webserv webserv;
  try
  {
    webserv.parse(argc, argv, const_cast<const char **>(envp));
    webserv.initServer();
    webserv.loop();
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Unexpected error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
