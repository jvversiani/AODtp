CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
DEBUG_FLAGS = -g -DDEBUG
LDFLAGS =

SRC_DIR = src
BIN_DIR = bin

SOURCES = $(wildcard $(SRC_DIR)/*.c)

TARGET = $(BIN_DIR)/AODjustify

all: $(TARGET)

$(BIN_DIR):
	mkdir -p $@

$(TARGET): $(SOURCES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SOURCES) $(LDFLAGS) -o $@


clean:
	rm -f $(BIN_DIR)/AODjustify

run: $(TARGET)
	@./$(TARGET) 6 Benchmark-2025/foo.iso8859-1.in 2>&1

.PHONY: all clean run