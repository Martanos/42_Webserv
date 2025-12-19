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
		Cgi/CgiEnv.cpp \
		Cgi/CgiExecutor.cpp \
		Cgi/CgiHandler.cpp \
		Cgi/CgiResponse.cpp \
		Config/ConfigFileReader.cpp \
		Config/ConfigParser.cpp \
		Config/ConfigTokeniser.cpp \
		Config/ConfigTranslator.cpp \
		Config/Directives.cpp \
		Config/Location.cpp \
		Config/Server.cpp \
		Config/ServerMap.cpp \
		Core/Client.cpp \
		Core/EpollManager.cpp \
		Core/ServerManager.cpp \
		Global/Logger.cpp \
		Global/MimeTypeResolver.cpp \
		Global/PerformanceMonitor.cpp \
		Http/Header.cpp \
		Http/HttpBody.cpp \
		Http/HttpHeaders.cpp \
		Http/HttpRequest.cpp \
		Http/HttpResponse.cpp \
		Http/HttpURI.cpp \
		MethodHandlers/DeleteMethodHandler.cpp \
		MethodHandlers/GetMethodHandler.cpp \
		MethodHandlers/IMethodHandler.cpp \
		MethodHandlers/MethodHandlerFactory.cpp \
		MethodHandlers/PostMethodHandler.cpp \
		MethodHandlers/PutMethodHandler.cpp \
		Wrappers/FileDescriptor.cpp \
		Wrappers/FileManager.cpp \
		Wrappers/ListeningSocket.cpp \
		Wrappers/SocketAddress.cpp

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
