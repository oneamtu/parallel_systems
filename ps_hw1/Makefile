CC = g++
SRCS = ./src/*.cpp
INC = ./src/
OPTS = -std=c++17 -Wall -Werror -lpthread -O3

EXEC = bin/prefix_scan

all: clean compile

compile:
	$(CC) $(SRCS) $(OPTS) -I$(INC) -o $(EXEC)

debug:
	$(CC) $(SRCS) $(OPTS) -DEBUG -I$(INC) -o $(EXEC) -g

clean:
	rm -f $(EXEC)
