CC = nvcc
SRCS = ./src/*.cpp #./src/*.cu
INC = ./src/
OPTS =

EXEC = bin/kmeans

all: clean compile

compile:
	$(CC) $(SRCS) $(OPTS) -I$(INC) -o $(EXEC)

debug:
	$(CC) $(SRCS) $(OPTS) -DDEBUG -I$(INC) -o $(EXEC) -g

clean:
	rm -f $(EXEC)
