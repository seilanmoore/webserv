#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

#include <string>

std::string getStatusMessage(int code);

std::string getMimeType(const std::string &path);

bool fileExists(const std::string &path);
bool isDirectory(const std::string &path);
std::string buildFullPath(const std::string &root, const std::string &uri,
                          const std::string &locationPath);

#endif
