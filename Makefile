CC = gcc
CFLAGS = -Wall -Wextra -pedantic
LDLIBS = -lncurses

OBJS = pong.o paddle.o net.o protocol.o
TARGET = pong

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS)

pong.o: pong.c paddle.h court.h net.h protocol.h
	$(CC) $(CFLAGS) -c pong.c

paddle.o: paddle.c paddle.h
	$(CC) $(CFLAGS) -c paddle.c

net.o: net.c net.h
	$(CC) $(CFLAGS) -c net.c

protocol.o: protocol.c protocol.h
	$(CC) $(CFLAGS) -c protocol.c

clean:
	rm -f $(TARGET) $(OBJS)
