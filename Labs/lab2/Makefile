CFLAGS=-g -W -Wall
OBJECTS=command.o main.o my_output.txt

lfcat: main.o command.o
	gcc -g -o $@ $^

main.o: main.c command.h
command.o: command.c command.h


clean:
	rm -f $(OBJECTS)