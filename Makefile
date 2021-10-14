CC = g++

CFLAGS = -g -Wall -O3
SRC := src
BUILD := build

LIBS = -pthread -lfabric


SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

all: clean benchmark

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)

benchmark: $(OBJECTS)
	$(CC) $(CFLAGS) $^ -o build/$@ $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -I$(SRC) -c $< -o $@