/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/10 13:44:26 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 13:44:49 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Config.hpp"

Config::Config()
{
}

Config::Config(const Config &other)
{
  (void)other;
}

Config &Config::operator=(const Config &other)
{
  if (this == &other)
    return *this;
  return *this;
}

Config::~Config()
{
}
