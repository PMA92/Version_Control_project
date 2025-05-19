CC=cc
CFLAGS=-Iinclude -Wall

SRC=src/main.c src/init.c
OUT=mockgit

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)
