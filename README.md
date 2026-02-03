*This project has been created as part of the 42 curriculum by smoore-a.*

# Webserv

## Description

**Webserv** is a custom HTTP server written in C++98. The goal of this project is to understand the underlying mechanisms of the web by building a fully functional web server from scratch, without relying on heavy frameworks or pre-existing HTTP libraries.

This server is designed to handle non-blocking I/O operations, manage multiple connections simultaneously, and serve static and dynamic content. It adheres to the HTTP/1.1 protocol standards and replicates the behavior of established servers like NGINX.

**Key Features:**
* **Non-blocking I/O:** Uses `poll()` to handle multiple file descriptors and client connections concurrently.
* **Multi-port Listening:** Capable of setting up multiple virtual servers on different ports (e.g., 8080, 8081, 8082) with unique configurations.
* **HTTP Methods:** Supports `GET`, `POST`, and `DELETE` requests.
* **CGI Support:** Executes scripts (Python, PHP, Ruby, Perl, Bash) via the Common Gateway Interface.
* **File Management:** Handles file uploads and downloads.
* **Custom Configuration:** Parsed from a `.conf` file to set routes, error pages, body size limits, and more.
* **Directory Listing:** automatic index generation for directories.
* **HTTP Redirections:** Supports 3xx redirects.

## Instructions

### Prerequisites
* A Unix-based operating system (Linux/macOS).
* `g++` compiler.
* `make`.

### Compilation
To compile the project, run the following command at the root of the repository:

```bash
make
```
This will generate the webserv executable.

Other available Makefile rules:

* **`make clean`**: Removes object files.

* **`make fclean`**: Removes object files and the executable.

* **`make re`**: Recompiles the project from scratch.

### Execution

Run the server by providing a configuration file. If no argument is provided, the server will use a basic configuration.

#### Standard usage:

```bash
./webserv <path_to_conf_file>
```

Once the server is running, you can access it via your web browser or `curl`(e.g., `http://localhost:8080`).

To stop the server, press `Ctrl+C`.

## Technical Features & Configuration

The server behavior is defined in the configuration file (e.g., `config/default.conf`).

**Supported directives include:**

* **server**: Defines a new server block.
* **listen**: Specifies the port to listen on.
* **server_name**: Sets the server name.
* **error_page**: Maps error codes (404, 500, etc.) to custom HTML files.
* **client_max_body_size**: Limits the size of client requests (useful for file uploads).
* **location**: Defines routing rules for specific URIs.
  * **limit_except**: Restricts HTTP methods (GET, POST, DELETE).
  * **root**: Defines the directory root for the request.
  * **autoindex**: Turns directory listing on or off.
  * **index**: Specifies default files (e.g., index.html).
  * **cgi_pass**: Maps file extensions to CGI interpreters.
  * **upload_store**: Directory to save uploaded files.

## Resources
[Understanding Sockets](https://beej.us/guide/bgnet/html/#broadcast-packetshello-world)

[HTTP Reference 1](https://datatracker.ietf.org/doc/html/rfc9110) - 
[HTTP Reference 2](https://datatracker.ietf.org/doc/html/rfc9112)

[README Syntax](https://docs.github.com/es/get-started/writing-on-github/getting-started-with-writing-and-formatting-on-github/basic-writing-and-formatting-syntax)

### AI Usage
AI tools were used to assist with specific parts of this project:
* **Debugging & Logic**: LLMs were consulted to help understand the nuances of the `poll()` lifecycle and to debug edge cases regarding non-blocking socket states.
* **Test generation**: AI was used to generate Python scripts (found in `cgi-bin/`) to test the server's ability to handle infinite loops, syntax errors, and standard output generation.
* **Documentation**: AI assisted in formatting this README and summarizing the configuration options.
