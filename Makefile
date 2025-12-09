#Nom de l'executable
EXEC = GestionRessources

#Compilateur
CC = gcc

#Options de compilation
CFLAGS = -Wall -Wextra -Werror -g -Isrc

#Librairies à l'édition des liens
LDFLAGS = -lncurses

#Fichier sources
SRCS = main.c \
	   ui.c \
	   manager.c \
	   process.c \
	   options.c \

#Fichiers objets
OBJS = $(SRCS:.c=.o)
 #Cibke par défaut
 all: $(EXEC)

 #Création de l'executable
 $(EXEC): $(OBJS) $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

 #Compilation des .c en .o
 %.o: %.c $(CC) $(CFLAGS) -c $< -o $@

 #Nettoyage
 clean: rm -f $(OBJS)