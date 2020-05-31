CC	= gcc
CFLAGS	= -I.

all: myshell
myshell: myshell.o
	$(CC) -o shell myshell.c
