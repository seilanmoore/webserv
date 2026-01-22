/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Types.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 20:00:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:11:49 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TYPES_HPP
#define TYPES_HPP

#include <sys/types.h>
#include <arpa/inet.h>

#include <vector>
#include <string>

// ============================================================================
// CGI State - Unified structure for CGI process management
// ============================================================================

struct CgiState
{
  pid_t pid;                   // CGI process ID
  int stdinFd;                 // Pipe to write to CGI stdin
  int stdoutFd;                // Pipe to read from CGI stdout
  int connectionFd;            // The connection waiting for this CGI
  std::vector<char> inputData; // Data to write to CGI
  size_t inputWritten;         // Bytes written so far
  std::string outputData;      // Data read from CGI
  bool stdinClosed;            // stdin pipe closed?
  bool stdoutClosed;           // stdout pipe closed (EOF)?
  bool active;                 // Is CGI currently active?
  int pollCycles;              // Poll cycles since CGI started (for timeout)

  CgiState();
};

// ============================================================================
// Poll FD Types
// ============================================================================

typedef enum ePollFDType
{
  SERVER,
  CONNECTION,
  CGI_STDIN,
  CGI_STDOUT
} tPollFDType;

// ============================================================================
// Server Configuration Helper
// ============================================================================

struct ServerConf
{
  std::string name;
  uint16_t port;
};

#endif
