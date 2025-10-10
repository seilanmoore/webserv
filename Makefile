# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: smoore-a <smoore-a@student.42malaga.com    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/09/08 14:07:03 by smoore-a          #+#    #+#              #
#    Updated: 2025/10/10 11:02:54 by smoore-a         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

CC = clang++ -Wall -Wextra -Werror -std=c++98 -g -DDEBUG
RM = rm -rf
NAME = ./webserv
NAME_SHORT = webserv

INC_DIR = inc
INC_UTIL_DIR = $(INC_DIR)/utils
INC_SERVER_DIR = $(INC_DIR)/server
INC_CLIENT_DIR = $(INC_DIR)/client
# INC_PARSE_DIR = $(INC_DIR)/parse

INC = -I$(INC_DIR) \
	-I$(INC_UTIL_DIR) \
	-I$(INC_SERVER_DIR) \
	-I$(INC_CLIENT_DIR) \
# 	-I$(INC_PARSE_DIR) \

SRC_DIR = src
UTILS_DIR = $(SRC_DIR)/utils
SERVER_DIR = $(SRC_DIR)/server
CLIENT_DIR = $(SRC_DIR)/client
# PARSE_DIR = $(SRC_DIR)/parse

OBJ_DIR = obj
OBJ_FILE = $(SRCS:$(SRC_DIR)%.cpp=$(OBJ_DIR)/%.o)

SRC_FILE = main.cpp \
	Webserv.cpp

UTIL_FILE = Reader.cpp \
	utils.cpp

CLIENT_FILE = Client.cpp \
	Request.cpp \
	Response.cpp \

SERVER_FILE = Server.cpp \
	CgiHandler.cpp

# PARSE_FILE = canonic_format.cpp \
# 	parse.cpp

SRCS =	$(addprefix $(SRC_DIR)/, $(SRC_FILE)) \
	$(addprefix $(UTILS_DIR)/, $(UTIL_FILE)) \
	$(addprefix $(CLIENT_DIR)/, $(CLIENT_FILE)) \
	$(addprefix $(SERVER_DIR)/, $(SERVER_FILE)) \

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

fclean : clean
	@$(RM) $(NAME)

re : fclean all
