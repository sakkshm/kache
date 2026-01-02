# ==== CONFIG ====
CXX := g++
SRC := $(wildcard src/*.cpp)

# Output directories
BUILD_DIR := build
DEV_DIR   := $(BUILD_DIR)/dev
PROD_DIR  := $(BUILD_DIR)/prod

# Compiler flags
CXXFLAGS_DEV  := -Wall -Wextra -std=c++17 -Og -g -DDEBUG
CXXFLAGS_PROD := -Wall -Wextra -std=c++17 -O3

# Main targets
TARGET_DEV  := $(DEV_DIR)/main
TARGET_PROD := $(PROD_DIR)/main

# Default target
all: prod

# Dev build (was debug)
dev:
	@mkdir -p $(DEV_DIR)
	@echo "Compiling dev version..."
	@for f in $(SRC); do \
		bin=$(DEV_DIR)/$$(basename $$f .cpp); \
		echo "  -> $$f -> $$bin"; \
		$(CXX) $(CXXFLAGS_DEV) $$f -o $$bin; \
	done
	@echo "Dev build done."

# Production build
prod:
	@mkdir -p $(PROD_DIR)
	@echo "Compiling production version..."
	@for f in $(SRC); do \
		bin=$(PROD_DIR)/$$(basename $$f .cpp); \
		echo "  -> $$f -> $$bin"; \
		$(CXX) $(CXXFLAGS_PROD) $$f -o $$bin; \
	done
	@echo "Production build done."

# Benchmark target (prod only)
benchmark: prod
	@echo "Running production benchmark..."
	@bash -c '\
		echo "Starting production server..."; \
		$(PROD_DIR)/main & \
		MAIN_PID=$$!; \
		echo "Main server PID: $$MAIN_PID"; \
		trap "kill -TERM $$MAIN_PID 2>/dev/null" EXIT; \
		$(PROD_DIR)/benchmark; \
		echo "Stopping main server..."; \
		kill -TERM $$MAIN_PID 2>/dev/null || true; \
		wait $$MAIN_PID 2>/dev/null || true; \
		echo "Benchmark complete, main server stopped." \
	'


# Cleanup
clean:
	@rm -rf $(BUILD_DIR)
	@echo "Cleaned build directories."
