/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:48 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/05 16:07:21 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "utils.hpp"

#include <fcntl.h>
#include <cstring>

#include <sstream> //itoa

#include <iostream>
#include <stdexcept>

void *ft_memset(void *ptr, int c, size_t n)
{
  unsigned char *byte_ptr = (unsigned char *)ptr;
  unsigned char byte_value = (unsigned char)c;

  for (size_t i = 0; i < n; i++)
    byte_ptr[i] = byte_value;

  return ptr;
}

void ft_strcpy(char *dst, const char *src)
{
  while (*src)
    *dst++ = *src++;
  *dst = '\0';
}

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
