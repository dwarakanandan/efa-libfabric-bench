CC = g++

CFLAGS = -g -Wall -O3
SRC := src
BUILD := build

LIBS = -pthread -lfabric

LFABRIC_SRC := -L/opt/amazon/efa/lib64/
LFABRIC_INCLUDES := -I/opt/amazon/efa/include/

SOURCES := $(wildcard $(SRC)/*.cpp)
OBJECTS := $(patsubst $(SRC)/%.cpp, $(BUILD)/%.o, $(SOURCES))

all: clean benchmark

clean:
	rm -rf $(BUILD)
	mkdir -p $(BUILD)

benchmark: $(OBJECTS)
	$(CC) $(CFLAGS) $(LFABRIC_SRC) $^ -o build/$@ $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	$(CC) $(CFLAGS) -I$(SRC) $(LFABRIC_INCLUDES) -c $< -o $@