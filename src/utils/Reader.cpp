/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Reader.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:30 by smoore-a          #+#    #+#             */
/*   Updated: 2025/09/08 14:07:33 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Reader.hpp"

#include <iostream>
#include <string>
#include <unistd.h>     // read()
#include <sys/socket.h> // recv()

#include "utils.hpp"

Reader::Reader() : buffer_size(4096)
{
  buffer = new char[buffer_size + 1]();
}

Reader::Reader(size_t buf_size) : buffer_size(buf_size)
{
  buffer = new char[buffer_size + 1]();
}

Reader::Reader(const Reader &other)
{
  buffer = new char[other.buffer_size + 1];
  for (size_t i = 0; i < other.buffer_size; ++i)
    buffer[i] = other.buffer[i];
  buffer[other.buffer_size] = '\0';
  buffer_size = other.buffer_size;
}

Reader &Reader::operator=(const Reader &other)
{
  if (this == &other)
    return *this;
  delete[] buffer;
  buffer = new char[other.buffer_size + 1];
  for (size_t i = 0; i < other.buffer_size; ++i)
    buffer[i] = other.buffer[i];
  buffer[other.buffer_size] = '\0';
  buffer_size = other.buffer_size;
  return *this;
}

Reader::~Reader()
{
  delete[] buffer;
}

bool Reader::readLine(int fd, std::string &line)
{
  size_t nlPos;
  ssize_t readBytes;

  line = std::string(buffer);
  while ((nlPos = line.find('\n')) == std::string::npos &&
         (readBytes = read(fd, buffer, buffer_size)) > 0)
  {
    buffer[readBytes] = '\0';
    line.append(buffer);
  }

  if (line.size() == 0)
    return false;

  if (nlPos != std::string::npos)
  {
    ft_memset(buffer, '\0', buffer_size + 1);
    ft_strcpy(buffer, line.c_str() + nlPos + 1);
    line.resize(nlPos);
  }
  else
    ft_memset(buffer, '\0', buffer_size + 1);

  return true;
}

bool Reader::recvLine(int fd, std::string &line, int flags)
{
  size_t nlPos;
  ssize_t readBytes;

  line = std::string(buffer);
  while ((nlPos = line.find('\n')) == std::string::npos &&
         (readBytes = recv(fd, buffer, buffer_size, flags)) > 0)
  {
    buffer[readBytes] = '\0';
    std::cout << "readBytes: " << readBytes << "\n";
    std::cout << "buffer: " << buffer << "\n";
    line.append(buffer);
  }

  if (line.size() == 0)
    return false;

  if (nlPos != std::string::npos)
  {
    ft_memset(buffer, '\0', buffer_size + 1);
    ft_strcpy(buffer, line.c_str() + nlPos + 1);
    line.resize(nlPos);
  }
  else
    ft_memset(buffer, '\0', buffer_size + 1);

  return true;
}
