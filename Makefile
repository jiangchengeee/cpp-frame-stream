# 没装 cmake 时的临时构建（装了 cmake 优先用 cmake）
CXX      ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2 -g -Iinclude
SRC      := $(wildcard src/*.cpp)
BIN      := frame_stream

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN)

run: $(BIN)
	./$(BIN)

test:
	$(CXX) $(CXXFLAGS) -pthread tests/test_bounded_queue.cpp -o /tmp/tq && /tmp/tq

clean:
	rm -rf $(BIN) build

.PHONY: all run test clean
