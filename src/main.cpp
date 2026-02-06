/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/01 11:24:30 by smoore-a          #+#    #+#             */
/*   Updated: 2026/02/05 11:22:53 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <iostream>
#include <stdexcept>
#include <signal.h>

#include "Webserv.hpp"

int main(int argc, char **argv, char **envp)
{
  // Ignore SIGPIPE to prevent crash when client closes connection early
  signal(SIGPIPE, SIG_IGN);

  Webserv webserv;
  try
  {
    webserv.parse(argc, argv, const_cast<const char **>(envp));
    webserv.initServer();
    webserv.loop();
  }
  catch (const std::runtime_error &e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
  catch (const std::exception &e)
  {
    std::cerr << "Unexpected error: " << e.what() << '\n';
    return 1;
  }
  return 0;
}
