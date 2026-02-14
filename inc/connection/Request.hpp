/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/10/02 19:24:42 by smoore-a          #+#    #+#             */
/*   Updated: 2026/02/14 13:08:25 by smoore-a         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <sys/types.h>
#include <cstddef>

#include <string>
#include <map>
#include <vector>

#define DEFAULT_MAX_CONTENT_LENGTH 8192

typedef enum eRecvStatus
{
    HEADER,
    BODY,
    DONE
} tRecvStatus;

class Request
{
public:
    Request();
    ~Request();

    void parse(const std::string &rawRequest);

    void setHeader(const char *buffer);
    void setBody(const char *buffer, ssize_t bytes);

    // Getters
    const tRecvStatus &getRecvStatus() const;
    const std::string &getHeader() const;
    const std::string &getFilename() const;
    const std::string &getContentType() const;
    const std::string &getQueryString() const;
    const std::string &getMethod() const;
    const std::string &getUri() const;
    const std::string &getPath() const;
    const std::string &getVersion() const;
    const std::string &getHost() const;
    size_t getContentLength() const;
    const std::vector<char> &getBody() const;
    std::vector<char> &getBodyMutable(); // For efficient move/swap operations
    const std::string &getFileExtension() const;
    bool getIsBinary() const;
    bool isChunked() const;
    bool isKeepAlive() const;
    const std::string &getTransferEncoding() const;
    const std::map<std::string, std::string> &getOtherHeaders() const;

private:
    bool decodeChunks();
    std::string _method;
    std::string _uri;
    std::string _version;

    std::string _host;
    bool _keepAlive;

    std::string _ifModifiedSince;
    std::string _ifNoneMatch;

    size_t _contentLength;
    std::string _contentType;
    std::string _transferEncoding;
    bool _isChunked;

    std::string _acceptEnconding;
    std::string _userAgent;

    std::map<std::string, std::string> _otherHeaders;

    std::string _header;
    std::vector<char> _body;

    tRecvStatus _recvStatus;

    size_t _bodyReadBytes;
    std::string _chunkBuffer;
    bool _chunkedDone;

    std::string _path;
    std::string _queryString;

    std::string _pathInfo;
    std::string _serverName;
    std::string _remoteAddr;

    std::string _fileExtension;
    std::string _range;

    std::string _filename;
    bool _isBinary;

    Request &operator=(const Request &other);
    Request(const Request &other);
};

#endif // REQUEST_HPP
