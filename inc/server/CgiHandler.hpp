#ifndef CGIHANDLER_HPP
#define CGIHANDLER_HPP

#include <map>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

void buildCgiExecArgBuffers(const std::string& scriptPath, char* argv[3], char interp_buf[256], char script_buf[256]);
void buildCgiEnv(const std::string& scriptPath, const std::string& queryString, const std::string& method, char* envp[4]);
void executeCGI(const std::string& scriptPath, const std::string& queryString, const std::string& method, int client_fd);
bool isCgiScript(const std::string& path);
std::string getCgiInterpreter(const std::string& extension);

#endif // CGIHANDLER_HPP
