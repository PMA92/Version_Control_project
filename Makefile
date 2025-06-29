CC=cc
CFLAGS=-Iinclude -Wall -I/opt/homebrew/opt/openssl@3/include 
LDFLAGS=-L/opt/homebrew/opt/openssl@3/lib -lcrypto

SRC=src/main.c src/init.c src/add.c src/commit.c src/log.c src/status.c src/branch.c
OUT=mockgit

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)
