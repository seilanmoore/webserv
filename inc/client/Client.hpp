/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/21 14:26:07 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 11:35:45 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <iostream>

#include "Request.hpp"
#include "Response.hpp"

class Client
{
public:
  Client();
  Client(const Client &other);
  Client &operator=(const Client &other);
  ~Client();

  int setupClient(int serverFD);

  ssize_t receiveRequest();

  void generateResponse();
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
