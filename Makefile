CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -lncurses

OBJS = pong.o paddle.o
TARGET = pong

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

pong.o: pong.c paddle.h
	$(CC) $(CFLAGS) -c pong.c

paddle.o: paddle.c paddle.h
	$(CC) $(CFLAGS) -c paddle.c

clean:
	rm -f $(TARGET) $(OBJS)
