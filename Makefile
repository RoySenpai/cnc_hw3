CC = gcc
CFLAGS = -Wall -g

.PHONY: all clean

all: Sender Receiver

Receiver: Receiver.o
	$(CC) $(CFLAGS) $< -o $@

Sender: Sender.o
	$(CC) $(CFLAGS) $< -o $@

Receiver.o: Receiver.c sockconst.h
	$(CC) $(CFLAGS) -c $<

Sender.o: Sender.c sockconst.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.gch Sender Receiver