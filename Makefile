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

benchmark: $(TARGET)
	printf "%-30s | %-10s | %-10s | %-10s\n" "Fichier" "Min" "Max" "Moyenne"; \
    	printf "%s\n" "-------------------------------------------------------------------------------------------"; \
    	for file in Benchmark-2025/*.in; do \
    		[ -f "$$file" ] || continue; \
    		min=""; max=""; total=0; count=0; \
    		i=1; \
    		while [ $$i -le 5 ]; do \
    			debut=$$(date +%s.%N); \
    			$(TARGET) 80 < "$$file" > /dev/null 2>&1; \
    			fin=$$(date +%s.%N); \
    			duree=$$(echo "$$fin - $$debut" | bc); \
    			if [ -z "$$min" ] || awk "BEGIN {exit !($$duree < $$min)}"; then min=$$duree; fi; \
    			if [ -z "$$max" ] || awk "BEGIN {exit !($$duree > $$max)}"; then max=$$duree; fi; \
    			total=$$(echo "$$total + $$duree" | bc); \
    			count=$$(($$count + 1)); \
    			i=$$(($$i + 1)); \
    		done; \
    		moyenne=$$(echo "scale=4; $$total / $$count" | bc); \
    		printf "%-30s | %-10s | %-10s | %-10s\n" "$$(basename "$$file")" "$$min" "$$max" "$$moyenne"; \
    	done; \

.PHONY: all clean test
