compile = gcc
flags = -Iinclude -Wall

mockgit: src/main.c src/init.c
	$(compile) $(flags) src/main.c src/init.c -o mockgit