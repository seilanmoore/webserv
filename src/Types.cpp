/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Types.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/22 16:25:22 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/22 16:27:29 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Types.hpp"

CgiState::CgiState()
	: pid(-1),
	  stdinFd(-1),
	  stdoutFd(-1),
	  connectionFd(-1),
	  inputData(),
	  inputWritten(0),
	  outputData(),
	  stdinClosed(false),
	  stdoutClosed(false),
	  active(false),
	  pollCycles(0)
{
}
