NAME = sample_ast
CC = gcc
CFLAGS = -Wall -Wextra -Werror -g

SRCS = main.c sample_lexer.c sample_parser.c sample_execute.c sample_print_ast.c sample_create_ast.c 
OBJS = $(SRCS:%.c=%.o)

# General rules

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $(NAME) $(OBJS)

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re test
