CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
SRC = src/main.c src/db.c src/git_integration.c src/notes.c src/sync.c src/verify.c
OUT = gitnote
LIBS = -lsqlite3

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)

install:
	cp $(OUT) /usr/local/bin/

uninstall:
	rm -f /usr/local/bin/$(OUT)