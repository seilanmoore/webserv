/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:07 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 11:35:45 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>

#include "Config.hpp"
#include "Request.hpp"
#include "Response.hpp"

class Connection
{
public:
  Connection();
  Connection(const Connection &other);
  Connection &operator=(const Connection &other);
  ~Connection();

  int setupConnection(int serverFD);

  ssize_t receiveRequest();

  void generateResponse(const ServerConfig &serverConfig);
  ssize_t sendServerResponse();

  void resetRequestResponse();

  uint16_t getPort() const;
  int getServerFD() const;

  const std::string &getResponse() const;

private:
  int _fd;
  struct sockaddr_in _address;
  int _serverFD;
  uint16_t _port;

  Request *_request;
  Response *_response;
};

#endif
