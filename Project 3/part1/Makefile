CC = gcc
CFLAGS = -Wall -pthread

TARGET = bank

SRCS = bank.c ../string_parser.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
	rm output.txt
