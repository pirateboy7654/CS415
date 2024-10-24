CFLAGS=-g -W -Wall
OBJECTS=command.o main.o string_parser.o

EXEC = pseudo-shell
pseudo-shell: $(OBJECTS)
	gcc $(CFLAGS) -o $(EXEC) $(OBJECTS)

# Compile main.o from main.c
main.o: main.c command.h string_parser.h
	gcc $(CFLAGS) -c main.c

# Compile command.o from command.c
command.o: command.c command.h
	gcc $(CFLAGS) -c command.c

# Compile string_parser.o from string_parser.c
string_parser.o: string_parser.c string_parser.h
	gcc $(CFLAGS) -c string_parser.c

# Clean target to remove object files and the executable
clean:
	rm -f $(OBJECTS) $(EXEC)


