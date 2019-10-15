#author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz

CC = gcc
CFLAGS = -Wall -Werror -std=c99 -pedantic
SRC = dns.c
BIN = dns

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

clean:
	rm -f *.o *.out $(BIN)
