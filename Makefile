#author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz

LOGIN = xgrenc00

CC = g++
CFLAGS = -std=c++11 -Wall -Werror -pedantic
LDFLAGS = -L/usr/local/ssl/lib
LDLIBS = -lssl -lcrypto

HEADER = isabot.hpp
SRC = isabot.cpp
BIN = isabot
DOC = README manual.pdf
PROJECT_FILES = $(SRC) $(HEADER) Makefile $(DOC)

$(BIN): $(SRC)
	$(CC) $(SRC) -o $(BIN) $(CFLAGS) $(LDFLAGS) $(LDLIBS)

debug: $(SRC)
	$(CC) $(SRC) -o $(BIN) $(CFLAGS) -g -DDEBUG=1 $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o *.out $(BIN) $(LOGIN).tar

tar: clean
	tar -cvf $(LOGIN).tar $(PROJECT_FILES)
