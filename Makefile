# ==== CONFIG ====
CXX := g++
CXXFLAGS := -Wall -Wextra -std=c++17 -Og -g
SRC := $(wildcard src/*.cpp)

# main binary:
TARGET := build/main

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p build
	@echo "Compiling single binary with all sources..."
	$(CXX) $(CXXFLAGS) $^ -o $@

# Build each source file as its own binary
each:
	@mkdir -p build
	@echo "Compiling each source file separately..."
	@for f in $(SRC); do \
		bin=build/$$(basename $$f .cpp); \
		echo "  -> $$f -> $$bin"; \
		$(CXX) $(CXXFLAGS) $$f -o $$bin; \
	done

# Cleanup
clean:
	rm -rf build
