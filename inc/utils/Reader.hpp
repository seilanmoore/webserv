/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Reader.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/09/08 14:07:26 by smoore-a          #+#    #+#             */
/*   Updated: 2025/09/11 12:24:41 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef READER_HPP
#define READER_HPP

#include <string>
#include <cstddef> // size_t typedef

class Reader
{
public:
  Reader();
  Reader(size_t buf_size);
  Reader(const Reader &other);
  Reader &operator=(const Reader &other);
  ~Reader();

  bool readLine(int fd, std::string &line);
  bool recvLine(int fd, std::string &line, int flags);

private:
  char *buffer;
  size_t buffer_size;
};

#endif
