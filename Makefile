SRC_DIR=src
LIB_DIR=lib
BUILD_DIR=build
BINARY=build/ZaklopaChessServer
PORT=8080

CXX=gcc
GDB=gdb
MKDIR=mkdir -p
RM=rm -rf

LDFLAGS=-lm
CXXFLAGS=-Wall -I $(LIB_DIR) -O3

SOURCES=$(wildcard $(SRC_DIR)/*.c)
OBJECTS=$(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

.PHONY: all run clean web
all: compile

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BINARY): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS)

compile:
	$(MKDIR) $(BUILD_DIR)
	$(MAKE) $(BINARY)

run: compile
	./$(BINARY) $(PORT)

clean:
	$(RM) $(BINARY) $(BUILD_DIR)
