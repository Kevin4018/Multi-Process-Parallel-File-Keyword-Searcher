CC = gcc

CFLAGS = -Wall -Wextra -std=gnu99 -g


OBJS = pfind.o worker.o util.o
TARGET = pfind


all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

pfind.o: pfind.c worker.h protocol.h util.h
	$(CC) $(CFLAGS) -c pfind.c

worker.o: worker.c worker.h protocol.h util.h
	$(CC) $(CFLAGS) -c worker.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm -f $(OBJS) $(TARGET)