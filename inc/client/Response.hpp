/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Response.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjeannin <mjeannin@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:27 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/01 15:07:49 by mjeannin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <sys/types.h>

#include <vector>

#include "Request.hpp"
#include "CgiHandler.hpp"

class Response
{
public:
  Response();
  Response(const Response &other);
  Response &operator=(const Response &other);
  ~Response();

  void generateResponse(const Request &r);

  ssize_t sendResponse(int fd);

  const std::string &getResponse() const;

private:
  bool _sendingBinary;
  ssize_t _binaryBytesSent;

  // std::string _buffer;
  ssize_t _bytesSent;

  std::string _contentType;

  std::string _filename;
  bool _fileFound;

  // std::ifstream _file;
  std::string _fileContent;

  bool _isBinary;
  // std::ifstream _binaryFile;
  std::vector<char> _binaryContent;

  ssize_t _contentLength;

  std::string _response;
};

#endif
