/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpUtils.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 19:00:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 18:59:37 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPUTILS_HPP
#define HTTPUTILS_HPP

#include <string>

namespace HttpUtils
{
  // HTTP status messages
  std::string getStatusMessage(int code);

  // MIME type detection
  std::string getMimeType(const std::string &path);

  // File system utilities
  bool fileExists(const std::string &path);
  bool isDirectory(const std::string &path);
  std::string buildFullPath(const std::string &root, const std::string &uri,
                            const std::string &locationPath);
}

#endif
