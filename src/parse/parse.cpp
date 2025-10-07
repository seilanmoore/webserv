#include "parse.hpp"
#include "canonic_format.cpp"
// int checkLine(std::string line, NodeType node){
// 	// if (find(line,node)){

// 	// }
// 	return 1;
// }
void createBlock(std::string name, std::string location,std::vector<Block> subBlockList, Directives* dirList);

void Config::setName(std::string name){
	this->_name = name;
};

void Config::setDir(Directives  directives){
	this->_dir = directives;
};

void Config::setBlockList(std::vector<Block> blockList){
	this->_blockList = blockList;
};

void Config::setNextConfig(Config* nextConfig){
	this->_nextConfig = nextConfig;
};

std::string Config::getName(){
	return (this->_name);
};

Directives Config::getDir(){
	return (this->_dir);
};

std::vector<Block> Config::getSubBlockList(){
	return (this->_blockList);
};

Config * Config::getNextConfig(){
	return (this->_nextConfig);
};

void Block::setName(std::string name){
	this->name = name;
};

void Block::setLocation(std::string location){
	this->location = location;
};

void Block::setDir(Directives  directives){
	this->dir = directives;
};

void Block::setBlockList(std::vector<Block> blockList){
	this->subBlockList = blockList;
};


std::string Block::getName(){
	return (this->name);
};

std::string Block::getLocation(){
	return (this->location);
};

Directives Block::getDir(){
	return (this->dir);
};

std::vector<Block> Block::getSubBlockList(){
	return (this->subBlockList);
};

void Directives::setCMBS(std::string val){
	this->clientMaxBodySize = val;
};

void Directives::setErrorPage(std::vector<std::string> val){
	this->errorPage = val;
};

void Directives::setListen(unsigned long* val){
	this->listen = val;
};

void Directives::setServerName(std::vector<std::string> val){
	this->serverName = val;
};

void Directives::setRoot(std::string val){
	this->root = val;
};

void Directives::setIndex(std::string val){
	this->index = val;
};

void Directives::setAutoIndex(bool* val){
	this->autoindex = val;
};

void Directives::setReturnCode(int val){
	this->returnCode = val;
};

void Directives::setLimitExcept(std::vector<eMethod> val){
	this->limit_except = val;
};

std::string Directives::getCMBS(){
	return(this->clientMaxBodySize);
};

std::vector<std::string> Directives::getErrorPage(){
	return(this->errorPage);
};

unsigned long* Directives::getListen(){
	return(this->listen);
};

std::vector<std::string> Directives::getServerName(){
	return(this->serverName);
};

std::string Directives::getRoot(){
	return(this->root);
};

std::string Directives::getIndex(){
	return(this->index);
};

bool* Directives::getAutoIndex(){
	return(this->autoindex);
};

int Directives::getReturnCode(){
	return(this->returnCode);
};

std::vector<eMethod> Directives::getLimitExcept(){
	return(this->limit_except);
};

static void printDirective(Directives arr, int depth){
	unsigned long i = 0;
	if (arr.clientMaxBodySize != "")
		std::cout << std::string(depth,' ')<< "client_max_body_size: " << arr.clientMaxBodySize << "\n";
	if (!arr.errorPage.empty()){
		std::cout << std::string(depth,' ')<< "error_page: ";
		while (i < arr.errorPage.size()){
			std::cout << arr.errorPage[i] << " ";
			i++;
		}
		std::cout << "\n";
	}
	if (arr.listen != NULL)
		std::cout << std::string(depth,' ')<< "listen: " << *arr.listen << "\n";
	if (!arr.serverName.empty()){
		std::cout << std::string(depth,' ')<< "server_name: ";
		i = 0;
		while (i < arr.serverName.size()){
			std::cout << arr.serverName[i] << " ";
			i++;
		}
		std::cout << "\n";
	}
	if (arr.root != "")
		std::cout << std::string(depth,' ')<< "root: " << arr.root << "\n";
	if (arr.index != "")
		std::cout << std::string(depth,' ')<< "index: " << arr.index << "\n";
	if (arr.autoindex != NULL)
		std::cout << std::string(depth,' ')<< "autoindex: " << (*arr.autoindex ? "on" : "off") << "\n";
	if (arr.returnCode > 0)
		std::cout << std::string(depth,' ')<< "return: " << arr.returnCode << "\n";
	if (!arr.limit_except.empty()){
		std::cout << std::string(depth,' ')<< "limit_except: ";
		i = 0;
		while (i < arr.limit_except.size()){
			if (arr.limit_except[i] == 0)
				std::cout << "GET ";
			else if (arr.limit_except[i] == 1)
				std::cout << "DELETE ";
			else
				std::cout << "POST ";
			i++;
		}
		std::cout << "\n";
	}
}

static void printBlock(Block blockList, int depth){
	unsigned long i = 0;
	std::cout << std::string(depth,' ') << blockList.name << " {\n";
	printDirective(blockList.dir,depth+4);
	while (i < blockList.subBlockList.size()){
		printBlock(blockList.subBlockList[i],depth + 4);
		i++;
	}
	std::cout << std::string(depth,' ') << "}\n";
}

void Config::printConfig(){
	unsigned long i = 0;
	std::cout << this->_name << " {\n";
	printDirective(this->_dir, 4);
	while (i < this->_blockList.size()){
		printBlock(this->_blockList[i], 4);
		i++;
	}
	std::cout << "}\n";

	if (this->_nextConfig != NULL){
		this->_nextConfig->printConfig();
	}
}

void Config::staticConfig(){
	Config mainCtx;
	Block block1;
	Block block2;	
	Directives CtxDirective;
	Directives Block1Directive;
	Directives Block2Directive;
	Directives Ctx2Directive;
	std::vector<std::string> ctx12List;
	std::vector<std::string> block12List;
	std::vector<std::string> block17List;
	std::vector<std::string> block24List;
	std::vector<std::string> ctx21List;
	std::vector<eMethod> block26List;
	ctx12List.push_back("404");
	ctx12List.push_back("https://www.google.es");
	block12List.push_back("example.es");
	block12List.push_back("test.com");
	block17List.push_back("404");
	block17List.push_back("/404.html");
	block24List.push_back("400");
	block24List.push_back("401");
	block24List.push_back("=404");
	block24List.push_back("error.html");
	block26List.push_back(GET);
	block26List.push_back(DELETE);
	ctx21List.push_back("www.example2.com");
	CtxDirective.clientMaxBodySize = "54m";
	CtxDirective.errorPage = ctx12List;
	Block1Directive.listen = new unsigned long(8080);
	Block1Directive.serverName = block12List;
	Block1Directive.root = "/var/www/html";
	Block1Directive.index = "index.html";
	Block1Directive.clientMaxBodySize = "42G";
	Block1Directive.autoindex = new bool(false);;
	Block1Directive.errorPage = block17List;
	Block1Directive.returnCode = 404;
	Block2Directive.root = "/var/www/html";
	Block2Directive.index = "index.html";
	Block2Directive.autoindex = new bool(true);
	Block2Directive.errorPage = block24List;
	Block2Directive.returnCode = 404;
	Block2Directive.limit_except = block26List;
	Ctx2Directive.listen = new unsigned long(443);
	Ctx2Directive.serverName = ctx21List;

	std::vector<Block> empty;

	block2.name = "location";
	block2.location = "/";
	block2.dir = Block2Directive;
	block2.subBlockList = empty;

	std::vector<Block>	blocklist;
	blocklist.push_back(block2);
	block1.name = "server";
	block1.location = "";
	block1.dir = Block1Directive;
	block1.subBlockList = blocklist;

	std::vector<Block> ctxBlockList;
	ctxBlockList.push_back(block1);

	Config* nextCtx = new Config();
	nextCtx->setName("server");
	nextCtx->setDir(Ctx2Directive);
	nextCtx->setBlockList(empty);
	nextCtx->setNextConfig(NULL);
	
	mainCtx.setName("http");
	mainCtx.setDir(CtxDirective);
	mainCtx.setBlockList(ctxBlockList);
	mainCtx.setNextConfig(nextCtx);
	this->_name = mainCtx.getName();
	this->_dir = mainCtx.getDir();
	this->_blockList = mainCtx.getSubBlockList();
	this->_nextConfig = mainCtx.getNextConfig() ? new Config(*mainCtx.getNextConfig()) : NULL;
}

int parseConfFile(const char* fileName){
	std::ifstream file(fileName);
	file.seekg (0, file.end);
	int length = file.tellg();
	file.seekg (0, file.beg);

	char * buffer = new char [length];

	std::cout << "Reading " << length << " characters... ";
	file.read (buffer,length);

	if (file)
		std::cout << "all characters read successfully.";
	file.close();
	// while (1){
	// 	checkLine(buffer,Config);
	// }
	std::cout << buffer << std::endl;
	delete[] buffer;
	return 0;
}
// int main(int argc, char **argv){
// 	if (argc != 2) {
// 		std::cerr << "Error: Invalid input\n";
// 		return (1);
// 	}
// 	(void)argv;
// 	// Directives newDirective;
// 	// std::vector<std::string> vectorArr;
// 	// vectorArr.push_back("aoc");
// 	// vectorArr.push_back("ao");
// 	// vectorArr.push_back("a");
// 	// newDirective.setDirective("paoc",vectorArr);
// 	// newDirective.setDirective("paoc",vectorArr);
// 	// newDirective.setDirective("paoc",vectorArr);
// 	// newDirective.printDirective();
// 	Parse newConfig;
// 	newConfig.staticConfig();
// 	newConfig.printConfig();
// }
