/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:48 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:54 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

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
