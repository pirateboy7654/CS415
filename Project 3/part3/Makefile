CC = gcc
CFLAGS = -pthread -g -lpthread

TARGET = bank

SRCS = pre_audit_bank.c ../string_parser.c

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)
	rm *.txt