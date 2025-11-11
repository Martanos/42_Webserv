NAME = webserv
CC = c++
CFLAGS = -Wall -Wextra -Werror -MMD -MP -O0 -g -pipe

# Default (release-ish) logging: show WARNING/ERROR/CRITICAL + ACCESS (LOG_MIN_LEVEL=2)
LOG_MIN_LEVEL?=2

# Append macro definition
CFLAGS += -DLOG_MIN_LEVEL=$(LOG_MIN_LEVEL)
STD = -std=c++98
MAKEFLAGS = -j$(shell nproc) --no-print-directory
# Directory structure
SRC_DIR = srcs
INC_DIR = includes
OBJ_DIR = obj/srcs
# Source files
SRC_FILES = main.cpp \
             1.ConfigParser/ConfigParser.cpp \
             1.ConfigParser/ConfigTokeniser.cpp \
             1.ConfigParser/ConfigFileReader.cpp \
             1.ConfigParser/ConfigTranslator.cpp \
             2.ServerMap/ServerMap.cpp \
             2.ServerMap/Server.cpp \
             2.ServerMap/Location.cpp \
             3.ServerManager/ServerManager.cpp \
             3.ServerManager/EpollManager.cpp \
             4.Client/Client.cpp \
			 5.HTTPmanagement/HttpURI.cpp \
			 5.HTTPmanagement/HttpHeaders.cpp \
			 5.HTTPmanagement/HttpBody.cpp \
			 5.HTTPmanagement/Header.cpp \
             5.HTTPmanagement/HttpRequest.cpp \
             5.HTTPmanagement/HttpResponse.cpp \
			MethodHandlers/IMethodHandler.cpp \
			MethodHandlers/GetMethodHandler.cpp \
			MethodHandlers/PostMethodHandler.cpp \
			MethodHandlers/DeleteMethodHandler.cpp \
			MethodHandlers/PutMethodHandler.cpp \
			MethodHandlers/MethodHandlerFactory.cpp \
			Wrappers/FileDescriptor.cpp \
			Wrappers/SocketAddress.cpp \
			Wrappers/FileManager.cpp \
			Wrappers/Logger.cpp \
			Wrappers/MimeTypeResolver.cpp \
			Wrappers/PerformanceMonitor.cpp \
			Wrappers/ListeningSocket.cpp \
			cgiexec/CgiEnv.cpp \
			cgiexec/CgiExecutor.cpp \
			cgiexec/CgiHandler.cpp \
			cgiexec/CgiResponse.cpp

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
	@printf "$(YELLOW)Compiling %s$(RESET)\r" "$@"
$(NAME): $(OBJ)
	@echo ""
	@echo "$(YELLOW)Linking $(NAME)...$(RESET)"
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
re:
	@$(MAKE) fclean
	@$(MAKE) all

# Debug target: enable full DEBUG level (LOG_MIN_LEVEL=0)
debug:
	@$(MAKE) fclean
	@$(MAKE) LOG_MIN_LEVEL=0 all
# Include dependency files
-include $(DEPS)
.PHONY: all clean fclean re
