CC = gcc
CXX = g++

CFLAGS = -Wall
CXXFLAGS = -Wall -std=c++17 -I./src -I/usr/include
LDFLAGS = -pthread

SRC = src
TEST = tests
BUILD = build
BIN = bin

SQLITE_SRC = $(SRC)/sqlite3.c
CPP_SRCS = $(filter-out $(SQLITE_SRC), $(wildcard $(SRC)/*.cpp))
TEST_SRCS = $(wildcard $(TEST)/*.cpp)

SQLITE_OBJ = $(BUILD)/sqlite3.o
CPP_OBJS = $(CPP_SRCS:$(SRC)/%.cpp=$(BUILD)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TEST)/%.cpp=$(BUILD)/%.o)

MAIN = $(BIN)/trading
TEST_BIN = $(BIN)/tests

$(BUILD):
	mkdir -p $(BUILD)

$(BIN):
	mkdir -p $(BIN)

$(SQLITE_OBJ): $(SQLITE_SRC) | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: $(SRC)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD)/%.o: $(TEST)/%.cpp | $(BUILD)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN): $(SQLITE_OBJ) $(CPP_OBJS) | $(BIN)
	$(CXX) $^ -o $@ $(LDFLAGS)

$(TEST_BIN): $(TEST_OBJS) $(SQLITE_OBJ) $(filter-out $(BUILD)/main.o, $(CPP_OBJS)) | $(BIN)
	$(CXX) $^ -o $@ $(LDFLAGS) -lgtest -lgtest_main

.PHONY: all clean test run rebuild

all: $(MAIN) $(TEST_BIN)

clean:
	rm -rf $(BUILD) $(BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

run: $(MAIN)
	./$(MAIN)

rebuild: clean all