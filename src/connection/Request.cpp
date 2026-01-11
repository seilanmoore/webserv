/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/02 20:00:09 by smoore-a          #+#    #+#             */
/*   Updated: 2026/01/10 20:59:45 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cctype>

#include "Request.hpp"

Request::Request()
    : _method(),
      _uri(),
      _version(),

      _host(),
      _keepAlive(true),

      _ifModifiedSince(),
      _ifNoneMatch(),

      _contentLength(0),
      _contentType(),
      _transferEncoding(),
      _isChunked(false),

      _acceptEnconding(),
      _userAgent(),

      _otherHeaders(),

      _header(),
      _body(),

      _recvStatus(HEADER),

      _bodyReadBytes(0),
      _chunkBuffer(),
      _chunkedDone(false),

      _path(),
      _queryString(),

      _pathInfo(),
      _serverName(),
      _remoteAddr(),

      _fileExtension(),
      _range(),

      _filename(),
      _isBinary(false)
{
}

Request::~Request()
{
}

void Request::parse(const std::string &rawRequest)
{
    std::istringstream iss(rawRequest);
    std::string line;

    // Parse request line (first line)
    if (std::getline(iss, line))
    {
        // Remove trailing \r if present
        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);

        std::istringstream firstLine(line);
        firstLine >> _method >> _uri >> _version;

        // Parse path and query string from URI
        _path = _uri;
        size_t qpos = _path.find('?');
        if (qpos != std::string::npos)
        {
            _queryString = _path.substr(qpos + 1);
            _path = _path.substr(0, qpos);
        }
        else
        {
            _queryString = "";
        }

        // Set file extension
        size_t dot = _path.find_last_of('.');
        size_t slash = _path.find_last_of('/');
        if (dot != std::string::npos && (slash == std::string::npos || dot > slash))
            _fileExtension = _path.substr(dot);
        else
            _fileExtension = "";
    }

    // Parse headers
    while (std::getline(iss, line))
    {
        // Remove trailing \r
        if (!line.empty() && line[line.length() - 1] == '\r')
            line.erase(line.length() - 1);

        if (line.empty())
            break;

        size_t sep = line.find(':');
        if (sep != std::string::npos)
        {
            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);
            // Trim leading whitespace from value
            size_t start = value.find_first_not_of(" \t");
            if (start != std::string::npos)
                value = value.substr(start);

            _otherHeaders[key] = value;

            // Handle specific headers
            if (key == "Host")
                _host = value;
            else if (key == "Content-Type")
                _contentType = value;
            else if (key == "Content-Length")
                _contentLength = static_cast<size_t>(std::atol(value.c_str()));
            else if (key == "Connection")
            {
                std::string lower = value;
                for (size_t i = 0; i < lower.length(); ++i)
                    lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
                _keepAlive = (lower != "close");
            }
            else if (key == "If-Modified-Since")
                _ifModifiedSince = value;
            else if (key == "If-None-Match")
                _ifNoneMatch = value;
            else if (key == "Range")
                _range = value;
            else if (key == "Accept-Encoding")
                _acceptEnconding = value;
            else if (key == "User-Agent")
                _userAgent = value;
            else if (key == "Transfer-Encoding")
            {
                _transferEncoding = value;
                // Check for chunked encoding (case-insensitive)
                std::string lower = value;
                for (size_t i = 0; i < lower.length(); ++i)
                    lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
                _isChunked = (lower.find("chunked") != std::string::npos);
            }
        }
    }
}

void Request::parseRequestLine(const std::string &line)
{
    std::istringstream iss(line);
    iss >> _method >> _uri >> _version;
}

void Request::parseHeader()
{
    parse(_header);
}

void Request::setHeader(const char *buffer, ssize_t readBytes)
{
    (void)readBytes;
    _header += buffer;
    std::size_t end = _header.find("\r\n\r\n");
    if (end != std::string::npos)
    {
        parseHeader();

        // Check if we have body data after headers
        std::string bodyData;
        if (_header.length() > end + 4)
            bodyData = _header.substr(end + 4);

        if (_isChunked)
        {
            // Chunked transfer encoding
            _recvStatus = BODY;
            if (!bodyData.empty())
            {
                _chunkBuffer += bodyData;
                if (decodeChunks())
                    _recvStatus = DONE;
            }
        }
        else if (_contentLength > 0)
        {
            _recvStatus = BODY;
            // Move any data after headers to body
            if (!bodyData.empty())
            {
                _body.insert(_body.end(), bodyData.begin(), bodyData.end());
                _bodyReadBytes = _body.size();
            }
            if (_body.size() >= _contentLength)
                _recvStatus = DONE;
        }
        else
            _recvStatus = DONE;
    }
}

void Request::setBody(const char *buffer, ssize_t readBytes)
{
    if (_isChunked)
    {
        // Add to chunk buffer and try to decode
        _chunkBuffer.append(buffer, static_cast<size_t>(readBytes));
        if (decodeChunks())
            _recvStatus = DONE;
    }
    else
    {
        _body.insert(_body.end(), buffer, buffer + readBytes);
        _bodyReadBytes += static_cast<size_t>(readBytes);
        if (_bodyReadBytes >= _contentLength)
            _recvStatus = DONE;
    }
}

// ============================================================================
// Chunked Transfer Encoding
// ============================================================================

bool Request::decodeChunks()
{
    // Parse chunked encoding format:
    // <chunk-size in hex>\r\n
    // <chunk-data>\r\n
    // ... repeat ...
    // 0\r\n
    // \r\n (optional trailer headers)

    while (true)
    {
        // Find the chunk size line
        size_t lineEnd = _chunkBuffer.find("\r\n");
        if (lineEnd == std::string::npos)
            return false; // Need more data

        // Parse hex chunk size
        std::string sizeLine = _chunkBuffer.substr(0, lineEnd);

        // Remove any chunk extensions (after ;)
        size_t semiPos = sizeLine.find(';');
        if (semiPos != std::string::npos)
            sizeLine = sizeLine.substr(0, semiPos);

        // Convert hex to size
        size_t chunkSize = 0;
        for (size_t i = 0; i < sizeLine.length(); ++i)
        {
            char c = sizeLine[i];
            chunkSize *= 16;
            if (c >= '0' && c <= '9')
                chunkSize += static_cast<size_t>(c - '0');
            else if (c >= 'a' && c <= 'f')
                chunkSize += static_cast<size_t>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                chunkSize += static_cast<size_t>(c - 'A' + 10);
            else
                break; // Invalid character, stop parsing
        }

        // Check if this is the final chunk (size 0)
        if (chunkSize == 0)
        {
            // Look for final CRLF after 0\r\n
            size_t trailerStart = lineEnd + 2;
            size_t finalCrlf = _chunkBuffer.find("\r\n", trailerStart);
            if (finalCrlf == std::string::npos)
                return false; // Need more data for trailer

            _chunkedDone = true;
            _contentLength = _body.size();
            return true;
        }

        // Check if we have enough data for this chunk
        size_t dataStart = lineEnd + 2;
        size_t chunkEnd = dataStart + chunkSize + 2; // +2 for trailing CRLF

        if (_chunkBuffer.size() < chunkEnd)
            return false; // Need more data

        // Extract chunk data and add to body
        const char *chunkData = _chunkBuffer.data() + dataStart;
        _body.insert(_body.end(), chunkData, chunkData + chunkSize);

        // Remove processed chunk from buffer
        _chunkBuffer.erase(0, chunkEnd);
    }

    return false;
}

// ============================================================================
// Getters
// ============================================================================

const tRecvStatus &Request::getRecvStatus() const
{
    return _recvStatus;
}

const std::string &Request::getHeader() const
{
    return _header;
}

const std::string &Request::getFilename() const
{
    return _filename;
}

const std::string &Request::getContentType() const
{
    return _contentType;
}

bool Request::getIsBinary() const
{
    return _isBinary;
}

const std::string &Request::getQueryString() const
{
    return _queryString;
}

const std::string &Request::getMethod() const
{
    return _method;
}

const std::string &Request::getUri() const
{
    return _uri;
}

const std::string &Request::getPath() const
{
    return _path;
}

const std::string &Request::getVersion() const
{
    return _version;
}

const std::string &Request::getHost() const
{
    return _host;
}

size_t Request::getContentLength() const
{
    return _contentLength;
}

const std::vector<char> &Request::getBody() const
{
    return _body;
}

const std::string &Request::getFileExtension() const
{
    return _fileExtension;
}

bool Request::isChunked() const
{
    return _isChunked;
}

bool Request::isKeepAlive() const
{
    return _keepAlive;
}

const std::string &Request::getTransferEncoding() const
{
    return _transferEncoding;
}

const std::map<std::string, std::string> &Request::getOtherHeaders() const
{
    return _otherHeaders;
}

// ============================================================================
// Private - Do not use
// ============================================================================

Request::Request(const Request &other)
{
    (void)other;
}

Request &Request::operator=(const Request &other)
{
    if (this == &other)
        return *this;
    return *this;
}
