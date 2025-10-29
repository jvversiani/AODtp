CC = gcc
CFLAGS = -Wall -Wextra -O2 -g
TARGET = bin/AODjustify
SRC = src/file_parser.c
BINDIR = bin

all: $(BINDIR) $(TARGET)

$(BINDIR):
	mkdir -p $(BINDIR)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
	rm -rf $(BINDIR)

test: $(TARGET)
	valgrind --tool=cachegrind $(TARGET) 80 Benchmark-2025/longtempsjemesuis.ISO-8859.in

.PHONY: all clean test