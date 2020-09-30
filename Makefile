#author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz

LOGIN = xgrenc00

CC = g++
CFLAGS = -Wall -Werror -pedantic -lm

HEADER = isabot.h
SRC = isabot.cpp
BIN = isabot
DOC = README manual.pdf
PROJECT_FILES = $(SRC) $(HEADER) Makefile $(DOC)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

debug: $(SRC)
	$(CC) $(CFLAGS) -g -DDEBUG=1 $(SRC) -o $(BIN)

clean:
	rm -f *.o *.out $(BIN) $(LOGIN).tar

tar: clean
	tar -cvf $(LOGIN).tar $(PROJECT_FILES)
