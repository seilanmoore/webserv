CC = c++ -Wall -Wextra -Werror -std=c++98 #-g -DDEBUG
RM = rm -rf
NAME = ./webserv
NAME_SHORT = webserv

INC_DIR = inc
INC_SERVER_DIR = $(INC_DIR)/server
INC_CONNECTION_DIR = $(INC_DIR)/connection
INC_UTIL_DIR = $(INC_DIR)/utils
INC_CONFIG_DIR = $(INC_DIR)/config

INC = -I$(INC_DIR) \
	-I$(INC_SERVER_DIR) \
	-I$(INC_CONNECTION_DIR) \
	-I$(INC_UTIL_DIR) \
	-I$(INC_CONFIG_DIR)

SRC_DIR = src
SERVER_DIR = $(SRC_DIR)/server
CONNECTION_DIR = $(SRC_DIR)/connection
UTILS_DIR = $(SRC_DIR)/utils
CONFIG_DIR = $(SRC_DIR)/config

OBJ_DIR = obj
OBJ_FILE = $(SRCS:$(SRC_DIR)%.cpp=$(OBJ_DIR)/%.o)

SRC_FILE = main.cpp \
	Webserv.cpp \
	WebservCgi.cpp \
	Types.cpp

UTIL_FILE = utils.cpp

CONNECTION_FILE = Connection.cpp \
	Request.cpp \
	Response.cpp \
	ResponseCgi.cpp \
	HttpUtils.cpp

SERVER_FILE = Server.cpp

CONFIG_FILE = Config.cpp

SRCS =	$(addprefix $(SRC_DIR)/, $(SRC_FILE)) \
	$(addprefix $(UTILS_DIR)/, $(UTIL_FILE)) \
	$(addprefix $(CONNECTION_DIR)/, $(CONNECTION_FILE)) \
	$(addprefix $(SERVER_DIR)/, $(SERVER_FILE)) \
	$(addprefix $(CONFIG_DIR)/, $(CONFIG_FILE))

_COLOR = \033[32m
_BOLDCOLOR = \033[32;1m
_RESET = \033[0m
_CLEAR = \033[0K\r\c
_OK = [\033[32mOK\033[0m]

$(OBJ_DIR)/%.o: $(SRC_DIR)%.cpp
	@mkdir -p $(dir $@)
	@echo -e "[..] $(NAME_SHORT)... compiling $<\r\c"
	@$(CC) $(INC) -c $< -o $@
	@echo -e "$(_CLEAR)"

all : $(NAME)

$(NAME) : $(OBJ_FILE)
	@$(CC) $(OBJ_FILE) $(INC) -o $(NAME)
	@echo -e "$(_OK) $(NAME_SHORT) compiled"

clean :
	@$(RM) $(OBJ_FILE)
	@$(RM) $(OBJ_DIR)
	@pkill $(NAME_SHORT) 2>/dev/null || true
	@sleep 0.2

fclean : clean
	@$(RM) $(NAME)

re : fclean all
