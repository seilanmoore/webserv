#include "parse.hpp"

Config::Config(){
	if (VERBOSE)
		std::cout << "Default Config constructor" << std::endl;
	_nextConfig = NULL;
};

Config::~Config() {
	if (VERBOSE)
		std::cout << "Default Config destructor" << std::endl;
	delete _nextConfig;
	_nextConfig = NULL;
};

Config::Config(const Config& other)
: _name(other._name),
_dir(other._dir),
_blockList(other._blockList),
  _nextConfig(other._nextConfig ? new Config(*other._nextConfig) : NULL) {
	if (VERBOSE)
		std::cout << "Config copy constructor" << std::endl;
};

Config& Config::operator=(const Config& other) {
	if (VERBOSE)
		std::cout << "Config assignment copy constructor" << std::endl;
	if (this != &other) {
		_name = other._name;
		_dir = other._dir;
		_blockList = other._blockList;
		_nextConfig = other._nextConfig ? new Config(*other._nextConfig) : NULL;
	}
	return *this;
};


Block::Block(){
	if (VERBOSE)
		std::cout << "Default block constructor" << std::endl;
};

Block::~Block() {
	if (VERBOSE)
		std::cout << "Default block destructor" << std::endl;
};

Block::Block(const Block& other)
: name(other.name), location(other.location),
  dir(other.dir),
  subBlockList(other.subBlockList)  {
	if (VERBOSE)
		std::cout << "Block copy constructor" << std::endl;
};

Block& Block::operator=(const Block& other) {
	if (VERBOSE)
		std::cout << "Block assignment copy constructor" << std::endl;
	if (this != &other) {
		name = other.name;
		subBlockList = other.subBlockList;
		dir = other.dir;
	}
	return *this;
}

Directives::Directives(){
	std::vector<std::string> nullVCT;
	this->clientMaxBodySize = "";
	this->errorPage.clear();
	this->listen = NULL;
	this->serverName.clear();
	this->root = "";
	this->index = "";
	this->autoindex = NULL;
	this->returnCode = 0;
	this->limit_except.clear();
	if (VERBOSE)
		std::cout << "Default directives constructor" << std::endl;
};

Directives::~Directives() {
	if (VERBOSE)
		std::cout << "Default directives destructor" << std::endl;
};

Directives::Directives(const Directives& other): clientMaxBodySize(other.clientMaxBodySize),errorPage(other.errorPage),listen(other.listen),serverName(other.serverName),root(other.root),index(other.index),autoindex(other.autoindex),returnCode(other.returnCode),limit_except(other.limit_except) {
	if (VERBOSE)
		std::cout << "Directives copy constructor" << std::endl;
};

Directives& Directives::operator=(const Directives& other) {
	if (VERBOSE)
		std::cout << "Directives assignment copy constructor" << std::endl;
	if (this != &other) {
		clientMaxBodySize = other.clientMaxBodySize;
		errorPage = other.errorPage;
		listen = other.listen;
		serverName = other.serverName;
		root = other.root;
		index = other.index;
		autoindex = other.autoindex;
		returnCode = other.returnCode;
		limit_except = other.limit_except;
	}
	return *this;
}
