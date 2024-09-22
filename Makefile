# OSに応じた設定を判別
UNAME_S := $(shell uname -s)

NAME = minishell
CC = cc
CFLAGS = -Wall -Wextra -Werror $(INCLUDES)

ifeq ($(UNAME_S),Darwin)
	# macOS用の設定
    RLDIR := $(shell brew --prefix readline)
    LIBS := -L$(RLDIR)/lib -lreadline
    INCLUDES := -I$(RLDIR)/include
else
	# Linux用の設定（readlineが標準でインストールされている）
    LIBS := -lreadline
    INCLUDES := 
endif

SRCS = src/main.c
OBJS = $(SRCS:src/%.c=src/%.o)

# General rules

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) $(LIBS) -o $(NAME) $(OBJS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

test: all
	./test.sh

.PHONY: all clean fclean re test