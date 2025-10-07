#ifndef PARSE_HPP
#define PARSE_HPP

#include <iostream>
#include <fstream>
#include <vector>
#include <utility>

#define VERBOSE false

// typedef enum NodeType {
// 	CONTEXT = 1,
// 	Block,
// 	DIRECTIVE
// };

// //This contains the name of a Block, including Contexts (anything with {})
// typedef enum BlockList{
// 	HTTP,
// 	SERVER,
// 	LOCATION
// };

// //This contains all of our Directives
enum DirList{
	CLIENT_MAX_BODY_SIZE,
	ERROR_PAGE,
	LISTEN,
	SERVER_NAME,
	ROOT,
	INDEX,
	AUTOINDEX,
	RETURN,
	LIMIT_EXCEPT
};


enum eMethod { GET, DELETE, POST };

struct sErrorPage
{
    std::vector<int> code;
    std::vector<std::string> path;
};

class Directives
{
	public:
		std::string clientMaxBodySize;
		std::vector<std::string> errorPage;
		unsigned long* listen;
		std::vector<std::string> serverName;
		std::string root;
		std::string index;
		bool* autoindex;
		int returnCode;
		std::vector<eMethod> limit_except;

		Directives();
		~Directives();
		Directives(const Directives& other);
		Directives& operator=(const Directives& other);

		void setCMBS(std::string val);
		void setErrorPage(std::vector<std::string> val);
		void setListen(unsigned long* val);
		void setServerName(std::vector<std::string> val);
		void setRoot(std::string val);
		void setIndex(std::string val);
		void setAutoIndex(bool* val);
		void setReturnCode(int val);
		void setLimitExcept(std::vector<eMethod> val);

		std::string getCMBS();
		std::vector<std::string> getErrorPage();
		unsigned long* getListen();
		std::vector<std::string> getServerName();
		std::string getRoot();
		std::string getIndex();
		bool* getAutoIndex();
		int getReturnCode();
		std::vector<eMethod> getLimitExcept();
};

class Block {
	public:
		std::string name;
		std::string location;
		Directives dir;
		std::vector<Block> subBlockList;

		Block();
		~Block();
		Block(const Block& other);
		Block& operator=(const Block& other);

		void setName(std::string name);
		void setLocation(std::string location);
		void setDir(Directives dir);
		void setBlockList(std::vector<Block> BlockList);

		std::string getName();
		std::string getLocation();
		Directives getDir();
		std::vector<Block> getSubBlockList();
};

class Config
{
	private:
		std::string _name;
		Directives _dir;
		std::vector<Block> _blockList;
		Config * _nextConfig;
	public:
		Config();
		~Config();
		Config(const Config& other);
		Config& operator=(const Config& other);

		void setName(std::string name);
		void setDir(Directives dir);
		void setBlockList(std::vector<Block> BlockList);
		void setNextConfig(Config* nextConfig);

		std::string getName();
		Directives getDir();
		std::vector<Block> getSubBlockList();
		Config * getNextConfig();

		void staticConfig();
		int	generateConf(std::string config);
		void printConfig();
};

#endif