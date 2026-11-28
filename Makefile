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

test-codec:
	$(CXX) $(CXXFLAGS) tests/test_frame_codec.cpp src/frame_codec.cpp -o /tmp/tfc && /tmp/tfc

test-buffer:
	$(CXX) $(CXXFLAGS) tests/test_buffer.cpp -o /tmp/tb && /tmp/tb

test-channel:
	$(CXX) $(CXXFLAGS) -pthread tests/test_channel.cpp src/channel.cpp src/event_loop.cpp -o /tmp/tch && /tmp/tch

test-poller:
	$(CXX) $(CXXFLAGS) tests/test_poller.cpp -o /tmp/tpl && /tmp/tpl

learn-6:
	$(CXX) $(CXXFLAGS) learn/06_epoll_echo.cpp -o /tmp/e6 && /tmp/e6

test-mini:
	$(CXX) $(CXXFLAGS) learn/test_mini_unique_ptr.cpp -o /tmp/tm && /tmp/tm

bench-pool:
	$(CXX) $(CXXFLAGS) -pthread tools/bench_pool.cpp -o /tmp/bp && /tmp/bp

test-pool:
	$(CXX) $(CXXFLAGS) -pthread tests/test_thread_pool.cpp -o /tmp/tp && /tmp/tp

test:
	$(CXX) $(CXXFLAGS) -pthread tests/test_bounded_queue.cpp -o /tmp/tq && /tmp/tq

clean:
	rm -rf $(BIN) build

.PHONY: all run test test-mini test-pool test-poller test-channel test-buffer test-codec bench-pool learn-6 clean
