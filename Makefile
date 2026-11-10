CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
FORMAT = clang-format
FORMATFLAGS = --Werror --dry-run
TARGET = ifj26
SRCS = $(wildcard *.c) test/tokenDump.c test/astPrint.c
OBJS = $(SRCS:.c=.o)

.PHONY: build check check-file build-check clean

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

HDRS = $(wildcard *.h) $(wildcard test/*.h)

check:
	$(FORMAT) $(FORMATFLAGS) $(SRCS) $(HDRS)

check-file:
	@test -n "$(FILE)" || { echo "Usage: make check-file FILE=<path>"; exit 1; }
	$(FORMAT) $(FORMATFLAGS) $(FILE)

build-check: check
	$(MAKE) build

clean:
	rm -f $(TARGET) $(OBJS)
