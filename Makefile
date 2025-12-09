#Nom de l'executable
EXEC = GestionRessources

#Compilateur
CC = gcc

#Options de compilation
CFLAGS =-Wall -Wextra -Werror -g -Iinclude
#Fichiers sources
SRCS=$(wildcard *.c)

 OBJS := $(patsubst %.c,%.o,$(SRCS))

 all: $(EXEC)
 $(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

 %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

 clean:
	rm -f $(OBJS) $(EXEC)

