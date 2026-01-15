/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Constants.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/14 20:00:00 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/14 19:11:49 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONSTANTS_HPP
#define CONSTANTS_HPP

#include <cstddef>

// ============================================================================
// File Paths
// ============================================================================

#define DEFAULT_CONFIG_FILE_PATH "./config/test.conf"

// ============================================================================
// Network Constants
// ============================================================================

// Maximum number of file descriptors to poll
static const size_t MAX_FD = 1021;

// Buffer size for reading/writing data
static const size_t BUFFER_SIZE = 65536;

// Poll timeout in milliseconds
static const int POLL_TIMEOUT_MS = 5000;

// ============================================================================
// HTTP Constants
// ============================================================================

// Chunk size for chunked transfer encoding
static const size_t CHUNK_SIZE = 8192;

// Threshold for using chunked encoding (10 MB)
static const size_t CHUNKED_THRESHOLD = 10485760;

// Default client max body size (1 MB)
static const size_t DEFAULT_CLIENT_MAX_BODY_SIZE = 1048576;

// ============================================================================
// CGI Constants
// ============================================================================

// CGI read/write buffer size
static const size_t CGI_BUFFER_SIZE = 65536;

#endif
