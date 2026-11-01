CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2
TARGET = ifj26
SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

.PHONY: build check build-check clean

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#TODO: check
check:


build-check: check
	$(MAKE) build

clean:
	rm -f $(TARGET) $(OBJS)
