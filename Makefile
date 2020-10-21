#author Radoslav Grenčík, xgrenc00@stud.fit.vutbr.cz

LOGIN = xgrenc00

CC = g++
CFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic
LDLIBS = -lssl -lcrypto

HEADER = isabot.hpp
SRC = isabot.cpp
BIN = isabot
DOC = README manual.pdf
PROJECT_FILES = $(SRC) $(HEADER) Makefile $(DOC)

$(BIN): $(SRC) $(HEADER)
	$(CC) $(SRC) -o $(BIN) $(CFLAGS) $(LDLIBS)

debug:
	$(CC) $(SRC) -o $(BIN) $(CFLAGS) -g -DDEBUG=1 $(LDLIBS)

clean:
	rm -f *.o *.out $(BIN) $(LOGIN).tar

tar: clean
	tar -cvf $(LOGIN).tar $(PROJECT_FILES)

upload:
	scp $(SRC) $(HEADER) Makefile eva:~/ISA
