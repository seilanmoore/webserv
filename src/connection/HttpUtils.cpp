/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpUtils.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 19:00:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 18:59:37 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpUtils.hpp"

#include <sys/stat.h>

namespace HttpUtils
{

  std::string getStatusMessage(int code)
  {
    switch (code)
    {
    case 200:
      return "OK";
    case 201:
      return "Created";
    case 204:
      return "No Content";
    case 301:
      return "Moved Permanently";
    case 302:
      return "Found";
    case 304:
      return "Not Modified";
    case 307:
      return "Temporary Redirect";
    case 308:
      return "Permanent Redirect";
    case 400:
      return "Bad Request";
    case 403:
      return "Forbidden";
    case 404:
      return "Not Found";
    case 405:
      return "Method Not Allowed";
    case 413:
      return "Payload Too Large";
    case 414:
      return "URI Too Long";
    case 500:
      return "Internal Server Error";
    case 501:
      return "Not Implemented";
    case 502:
      return "Bad Gateway";
    case 504:
      return "Gateway Timeout";
    default:
      return "Unknown";
    }
  }

  std::string getMimeType(const std::string &path)
  {
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
      return "application/octet-stream";

    std::string ext = path.substr(dot);

    // Text types
    if (ext == ".html" || ext == ".htm")
      return "text/html";
    if (ext == ".css")
      return "text/css";
    if (ext == ".js")
      return "application/javascript";
    if (ext == ".json")
      return "application/json";
    if (ext == ".xml")
      return "application/xml";
    if (ext == ".txt")
      return "text/plain";

    // Image types
    if (ext == ".png")
      return "image/png";
    if (ext == ".jpg" || ext == ".jpeg")
      return "image/jpeg";
    if (ext == ".gif")
      return "image/gif";
    if (ext == ".ico")
      return "image/x-icon";
    if (ext == ".svg")
      return "image/svg+xml";
    if (ext == ".webp")
      return "image/webp";

    // Audio/Video
    if (ext == ".mp3")
      return "audio/mpeg";
    if (ext == ".mp4")
      return "video/mp4";
    if (ext == ".webm")
      return "video/webm";

    // Documents
    if (ext == ".pdf")
      return "application/pdf";
    if (ext == ".zip")
      return "application/zip";

    return "application/octet-stream";
  }

  bool fileExists(const std::string &path)
  {
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
  }

  bool isDirectory(const std::string &path)
  {
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
      return false;
    return S_ISDIR(st.st_mode);
  }

  std::string buildFullPath(const std::string &root, const std::string &uri,
                            const std::string &locationPath)
  {
    std::string path = root;

    // Remove trailing slash from root if present
    if (!path.empty() && path[path.length() - 1] == '/')
      path.erase(path.length() - 1);

    // Get the part of URI after the location path
    std::string relativePath = uri;
    if (locationPath != "/" && uri.find(locationPath) == 0)
    {
      relativePath = uri.substr(locationPath.length());
      // If root already contains the location-like path, don't add uri part that would duplicate
      // e.g., root="./cgi-bin", location="/cgi-bin", uri="/cgi-bin/fortune.py"
      // We want: ./cgi-bin/fortune.py, not ./cgi-bin/cgi-bin/fortune.py
    }

    // Ensure relative path starts with /
    if (!relativePath.empty() && relativePath[0] != '/')
      relativePath = "/" + relativePath;

    return path + relativePath;
  }

} // namespace HttpUtils
