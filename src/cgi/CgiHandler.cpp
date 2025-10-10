#include <cstdio>
#include "CgiHandler.hpp"


bool isCgiScript(const std::string& path) {
	size_t dotPos = path.find_last_of('.');
	if (dotPos == std::string::npos)
		return false;
	std::string extension = path.substr(dotPos);
	return !getCgiInterpreter(extension).empty();
}

// Builds argv buffers for execve based on script path and extension mapping
void buildCgiExecArgBuffers(const std::string& scriptPath, char* argv[3], char interp_buf[256], char script_buf[256])
{
	size_t dotPos;
	std::string extension;
	std::string interpreter;
	dotPos = scriptPath.find_last_of('.');
	extension = (dotPos != std::string::npos) ? scriptPath.substr(dotPos) : "";
	interpreter = getCgiInterpreter(extension);
	strncpy(script_buf, scriptPath.c_str(), 255);
	script_buf[255] = '\0';
	if (!interpreter.empty()) {
		strncpy(interp_buf, interpreter.c_str(), 255);
		interp_buf[255] = '\0';
		argv[0] = interp_buf;
		argv[1] = script_buf;
		argv[2] = NULL;
	} else {
		argv[0] = script_buf;
		argv[1] = NULL;
		argv[2] = NULL;
	}
}

// Builds the environment array for execve based on CGI parameters
void buildCgiEnv(const std::string& scriptPath, const std::string& queryString, const std::string& method, char* envp[4])
{
	std::string req_method = "REQUEST_METHOD=" + method;
	std::string query_str = "QUERY_STRING=" + queryString;
	std::string script_filename = "SCRIPT_FILENAME=" + scriptPath;
	envp[0] = const_cast<char*>(req_method.c_str());
	envp[1] = const_cast<char*>(query_str.c_str());
	envp[2] = const_cast<char*>(script_filename.c_str());
	envp[3] = NULL;
}
std::string getCgiInterpreter(const std::string& extension)
{
	static std::map<std::string, std::string> extToInterpreter;
	if (extToInterpreter.empty()) {
		extToInterpreter[".py"] = "/usr/bin/python3";
		extToInterpreter[".php"] = "/usr/bin/php-cgi";
		extToInterpreter[".pl"] = "/usr/bin/perl";
	}
	std::map<std::string, std::string>::const_iterator it = extToInterpreter.find(extension);
	if (it != extToInterpreter.end()) {
		return it->second;
	}
	return "";
}

void executeCGI(const std::string& scriptPath, const std::string& queryString, const std::string& method, int client_fd)
{
	int pipefd[2];
	pid_t pid;
	char* envp[4];
	char interp_buf[256];
	char script_buf[256];
	char* argv[3];
	char buffer[4096];
	ssize_t n;

	if (pipe(pipefd) == -1)
	{
		std::cerr << "Pipe error" << std::endl;
		return;
	}

	pid = fork();
	if (pid == -1)
	{
		std::cerr << "Fork error" << std::endl;
		return;
	}
	if (pid == 0)
	{
		// Child process
		close(pipefd[0]); // Close read end
		dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to pipe
		close(pipefd[1]);

		buildCgiEnv(scriptPath, queryString, method, envp);
		buildCgiExecArgBuffers(scriptPath, argv, interp_buf, script_buf);
		execve(argv[0], argv, envp);
		// If exec fails
		std::cerr << "Exec error" << std::endl;
		perror("execve");
		exit(1);
	} else
	{
		close(pipefd[1]);
		while ((n = read(pipefd[0], buffer, sizeof(buffer)-1)) > 0)
		{
			write(client_fd, buffer, n);
		}
		close(pipefd[0]);
		waitpid(pid, NULL, 0);
	}
}

