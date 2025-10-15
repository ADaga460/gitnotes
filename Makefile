CC = gcc
CFLAGS = -Wall -Wextra -std=c11
SRC = src/main.c src/db.c
OUT = clisuite
LIBS = -lsqlite3

all:
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LIBS)

clean:
	rm -f $(OUT)
