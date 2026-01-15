/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:41 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:47:54 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
#define UTILS_HPP

#include <unistd.h>

#include <iostream>
#include <string>

//////////////////////////////////////
// MACRO TEMPORAL HECHA POR CHATGPT //
//////////////////////////////////////

#ifdef DEBUG
#include <iostream>
#include <cassert>
// Imprimir mensajes de debug
#define DEBUG_PRINT(msg)                                                 \
  do                                                                     \
  {                                                                      \
    std::cerr << msg << std::endl;                                       \
    std::cerr << "[DEBUG] " << __FILE__ << ":" << __LINE__ << std::endl; \
  } while (0)
// Assert de debug
#define DEBUG_ASSERT(cond) assert(cond)
// Imprimir variable (tipo simple)
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

//////////////////////////////////////

#include "Constants.hpp"

const std::string errorStr(int code);

int makeSocketNonBlocking(int socketFD);

#endif
