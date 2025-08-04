# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: seayeo <seayeo@42.sg>                      +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/07/24 16:54:44 by malee             #+#    #+#              #
#    Updated: 2025/08/04 19:11:36 by seayeo           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -MMD -MP
STD = -std=c++98
# Directory structure
SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = obj/srcs
# Source files
SRC_FILES = main.cpp \
			configparser/utils/ConfigParser.cpp \
			configparser/utils/ServerConfig.cpp \
			configparser/utils/LocationConfig.cpp \
			# Add other source files here as needed
# Object files with proper path
OBJ = $(addprefix $(OBJ_DIR)/, $(SRC_FILES:.cpp=.o))
# Dependency files
DEPS = $(OBJ:.o=.d)
# Color codes
GREEN = \033[0;32m
YELLOW = \033[0;33m
RED = \033[0;31m
RESET = \033[0m
all: $(OBJ_DIR) $(NAME)
# Rule to create obj directory structure
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)
# Modified object file rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(STD) -I$(INC_DIR) -c $< -o $@
	@echo "$(YELLOW)Compiling $@$(RESET)"
$(NAME): $(OBJ)
	@echo "$(YELLOW)Compiling $(NAME)...$(RESET)"
	@$(CC) $(CFLAGS) $(STD) $(OBJ) -o $(NAME)
	@echo "$(GREEN)Done!$(RESET)"
clean:
	@echo "$(RED)Deleting object files...$(RESET)"
	@if [ -d obj ]; then \
		rm -rf obj; \
		echo "$(GREEN)Object files deleted!$(RESET)"; \
	else \
		echo "$(YELLOW)No object files to delete!$(RESET)"; \
	fi
fclean:
	@echo "$(RED)Full clean $(NAME)...$(RESET)"
	@if [ -d obj ]; then \
		rm -rf obj; \
		echo "$(GREEN)Object files deleted!$(RESET)"; \
	else \
		echo "$(YELLOW)No object files to delete!$(RESET)"; \
	fi
	@if [ -f $(NAME) ]; then \
		rm -f $(NAME); \
		echo "$(GREEN)$(NAME) deleted!$(RESET)"; \
	else \
		echo "$(YELLOW)$(NAME) not found!$(RESET)"; \
	fi
	@echo "$(GREEN)Done!$(RESET)"
re: fclean all
# Include dependency files
-include $(DEPS)
.PHONY: all clean fclean re
