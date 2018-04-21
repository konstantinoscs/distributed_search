CC = gcc
FLAGS = -Wall
DEPS = plist.h trie.h utilities.h worker.h
NAME = jobExecutor
SOURCE = main.c plist.c trie.c utilities.c worker.c
OBJ = $(SOURCE:.c=.o)

%.o : %.c
	$(CC) -c $< -O0 -g

$(NAME): $(OBJ)
	$(CC) $(FLAGS) -o $(NAME) $(OBJ) -lm -O0 -g

$(OBJ): $(DEPS)

.PHONY: clean

clean:
	rm *.o $(NAME)
