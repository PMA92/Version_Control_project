CC=gcc
CFLAGS=-Iinclude -Wall

minigit: src/main.c src/init.c
	$(CC) $(CFLAGS) src/main.c src/init.c -o minigit
