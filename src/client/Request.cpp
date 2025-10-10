/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/02 20:00:09 by smoore-a          #+#    #+#             */
/*   Updated: 2025/10/10 12:00:32 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>

#include "Request.hpp"

Request::Request()
    : _method(),
      _uri(),
      _version(),

      _host(),
      _keepAlive(false),

      _ifModifiedSince(),
      _ifNoneMatch(),

      _contentLength(0),
      _contentType("text/html"),

      _acceptEnconding(),
      _userAgent(),

      _otherHeaders(),

      _header(),
      _body(),

      _recvStatus(HEADER),

      _bodyReadBytes(0),

      _path(),
      _queryString(),

      _pathInfo(),
      _serverName(),
      _remoteAddr(),

      _fileExtension(),
      _range(),

      _filename("docs/html/index.html"),
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
    // Parse first line
    if (std::getline(iss, line))
    {
        std::istringstream firstLine(line);
        firstLine >> _method >> _path >> _version;
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
    }
    // Parse headers
    while (std::getline(iss, line))
    {
        if (line.empty() || line == "\r")
            break;
        size_t sep = line.find(':');
        if (sep != std::string::npos)
        {
            std::string key = line.substr(0, sep);
            std::string value = line.substr(sep + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            _otherHeaders[key] = value;
            // Fill extra fields if present
            if (key == "Content-Type")
                _contentType = value;
            // if (key == "Content-Length")
            //     _contentLength = value;
            if (key == "If-Modified-Since")
                _ifModifiedSince = value;
            if (key == "Range")
                _range = value;
            if (key == "Host")
                _serverName = value;
        }
    }

    // Set fileExtension from path
    size_t dot = _path.find_last_of('.');
    if (dot != std::string::npos)
        _fileExtension = _path.substr(dot);
    else
        _fileExtension = "";

    // pathInfo: not handled here, but could be set if needed

    // remoteAddr: to be set externally (from socket info)

    // Parse body if present (for POST/PUT)
    // size_t bodyPos = rawRequest.find("\r\n\r\n");
    // if (bodyPos != std::string::npos)
    //     _body = rawRequest.substr(bodyPos + 4);
}

// bool Request::parseMessage()
// {
//     if (_header.find("\r\n\r\n") != std::string::npos)
//     {
//         if (_header.find("GET /about") == 0)
//             _filename = "docs/html/about.html";
//         else if (_header.find("GET /makelele") == 0)
//             _filename = "docs/html/makelele.html";
//         else if (_header.find("GET /png/makelele.png") == 0)
//         {
//             _filename = "docs/png/makelele.png";
//             _contentType = "image/png";
//             _isBinary = true;
//         }
//         return true;
//     }
//     return false;
// }

void Request::parseHeader()
{
    // std::istringstream iss(_header);
    // std::string line;

    // if (std::getline(iss, line))
    // {
    //     std::istringstream requestLine(line);
    //     requestLine >> _method >> _uri >> _version;
    // }

    if (_header.find("GET /about") == 0)
        _filename = "docs/html/about.html";
    else if (_header.find("GET /makelele") == 0)
        _filename = "docs/html/makelele.html";
    else if (_header.find("GET /png/makelele.png") == 0)
    {
        _filename = "docs/png/makelele.png";
        _contentType = "image/png";
        _isBinary = true;
    }
}

void Request::setHeader(const char *buffer, ssize_t readBytes)
{
    (void)readBytes;
    _header += buffer;
    std::size_t end = _header.find("\r\n\r\n");
    if (end != std::string::npos)
    {
        std::cerr << "entro!\n";
        parseHeader();
        if (_contentLength)
        {
            _recvStatus = BODY;
            _body.insert(_body.end(), _header.begin() + end + 4, _header.end());
            if (_body.size() == _contentLength)
                _recvStatus = DONE;
        }
        else
            _recvStatus = DONE;
    }
}

void Request::setBody(const char *buffer, ssize_t readBytes)
{
    _body.insert(_body.end(), buffer, buffer + readBytes);
    _bodyReadBytes += readBytes;
    if (_bodyReadBytes == _contentLength)
        _recvStatus = DONE;
}

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

///// DO NOT USE ////////

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
