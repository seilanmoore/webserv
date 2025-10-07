/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: mjeannin <mjeannin@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/01 11:24:30 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/01 15:09:31 by mjeannin         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <exception>
#include <iostream>
#include <stdexcept>

#include "Webserv.hpp"

int main(int argc, char **argv, char **envp)
{
  (void)envp;
  Webserv webserv;
  try
  {
    webserv.parse(argc, argv);
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
