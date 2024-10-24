CFLAGS=-g -W -Wall
OBJECTS=command.o main.o string_parser.o

# executable file
EXEC = pseudo-shell
pseudo-shell: $(OBJECTS)
	gcc $(CFLAGS) -o $(EXEC) $(OBJECTS)

# gcc main.o from main.c
main.o: main.c command.h string_parser.h
	gcc $(CFLAGS) -c main.c

# gcc command.o from command.c
command.o: command.c command.h
	gcc $(CFLAGS) -c command.c

# gcc string_parser.o from string_parser.c
string_parser.o: string_parser.c string_parser.h
	gcc $(CFLAGS) -c string_parser.c

# remove object files and executable
clean:
	rm -f $(OBJECTS) $(EXEC)


