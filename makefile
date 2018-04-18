CC = gcc
FLAGS = -Wall
DEPS = plist.h search.h trie.h utilities.h
NAME = jobExecutor
SOURCE = main.c plist.c search.c trie.c utilities.c
OBJ = $(SOURCE:.c=.o)

%.o : %.c
	$(CC) -c $<

$(NAME): $(OBJ)
	$(CC) $(FLAGS) -o $(NAME) $(OBJ) -lm

$(OBJ): $(DEPS)

.PHONY: clean

clean:
	rm *.o $(NAME)
