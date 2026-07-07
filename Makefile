CC       = gcc
CFLAGS   = -Wall -Wextra -Werror 
IFLAGS   = -Iincludes

SRC_DIR  = src
OBJ_DIR  = obj
INC_DIR  = includes

SRC      =	ft_ping.c \
			packet.c \
			socket.c \
			ping_send.c \
			ping_recv.c \
			ping_loop.c \
			debug.c \
			parsing.c

OBJECTS  = $(SRC:%.c=$(OBJ_DIR)/%.o)
HEADERS  = $(INC_DIR)/ft_ping.h

GREEN    = \033[0;32m
RED      = \033[0;31m
YELLOW   = \033[0;33m
RESET    = \033[0m

all: banner ft_ping

banner:
	@echo "$(GREEN)"
	@echo "███████╗████████╗    ██████╗ ██╗███╗   ██╗ ██████╗ "
	@echo "██╔════╝╚══██╔══╝    ██╔══██╗██║████╗  ██║██╔════╝ "
	@echo "█████╗     ██║       ██████╔╝██║██╔██╗ ██║██║  ███╗"
	@echo "██╔══╝     ██║       ██╔═══╝ ██║██║╚██╗██║██║   ██║"
	@echo "██║        ██║       ██║     ██║██║ ╚████║╚██████╔╝"
	@echo "╚═╝        ╚═╝       ╚═╝     ╚═╝╚═╝  ╚═══╝ ╚═════╝ "
	@echo "$(RESET)"

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(OBJ_DIR)
	@echo "$(YELLOW)Compiling $<...$(RESET)"
	@$(CC) $(CFLAGS) $(IFLAGS) -c $< -o $@

ft_ping: $(OBJECTS)
	@$(CC) $(CFLAGS) -o $@ $^ -lm
	@echo "$(GREEN)✔ ft_ping built successfully!$(RESET)"

clean:
	@rm -f $(OBJECTS)
	@echo "$(YELLOW)"
	@echo "██████╗██╗     ███████╗ █████╗ ███╗   ██╗"
	@echo "██╔═══╝██║     ██╔════╝██╔══██╗████╗  ██║"
	@echo "██║    ██║     █████╗  ███████║██╔██╗ ██║"
	@echo "██║    ██║     ██╔══╝  ██╔══██║██║╚██╗██║"
	@echo "██████╗███████╗███████╗██║  ██║██║ ╚████║"
	@echo "╚═════╝╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝"
	@echo "$(RESET)"
	@echo "$(YELLOW)Object files removed.$(RESET)"

fclean: clean
	@rm -f ft_ping
	@rm -rf $(OBJ_DIR)
	@echo "$(RED)"
	@echo "███████╗ ██████╗██╗     ███████╗ █████╗ ███╗   ██╗"
	@echo "██╔════╝██╔════╝██║     ██╔════╝██╔══██╗████╗  ██║"
	@echo "█████╗  ██║     ██║     █████╗  ███████║██╔██╗ ██║"
	@echo "██╔══╝  ██║     ██║     ██╔══╝  ██╔══██║██║╚██╗██║"
	@echo "██║     ╚██████╗███████╗███████╗██║  ██║██║ ╚████║"
	@echo "╚═╝      ╚═════╝╚══════╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝"
	@echo "$(RESET)"
	@echo "$(RED)ft_ping and object files removed.$(RESET)"

re: fclean all

.PHONY: all clean fclean re banner