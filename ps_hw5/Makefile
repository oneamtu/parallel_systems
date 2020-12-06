CC = mpic++
SRCS = ./src/*.cpp
INC = ./src/
LIBS = -lglfw -lGL
OPTS = --std=c++17

EXEC = bin/nbody

all: clean compile

compile:
	$(CC) -O3 $(SRCS) $(OPTS) -I$(INC) $(LIBS) -o $(EXEC)

debug:
	$(CC) $(SRCS) $(OPTS) -DDEBUG -I$(INC) $(LIBS) -o $(EXEC) -g

timing:
	$(CC) -O3 $(SRCS) $(OPTS) -DTIMING -I$(INC) $(LIBS) -o $(EXEC) -g

clean:
	rm -f $(EXEC)
