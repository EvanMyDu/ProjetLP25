#Nom de l'executable
EXEC = GestionRessources

#Compilateur
CC = gcc

#Options de compilation
CFLAGS =-Wall -Wextra -Werror -g -Iheader -Isrc
#Fichiers sources
SRCS=$(wildcard src/*.c) main.c

 OBJS := $(patsubst src/%.c,obj/%.o,$(SRCS))

 all: $(EXEC)
 $(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lncurses

 obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

 clean:
	rm -f $(OBJS) $(EXEC)

