CFLAGS = -Wall -g -w
OBJECTS = part1 part2 part3 part4 iobound cpubound

all: $(OBJECTS)

part1: part1.c
	gcc $(CFLAGS) -o part1 part1.c

part2: part2.c
	gcc $(CFLAGS) -o part2 part2.c

part3: part3.c
	gcc $(CFLAGS) -o part3 part3.c

part4: part4.c
	gcc $(CFLAGS) -o part4 part4.c

iobound: iobound.c
	gcc $(CFLAGS) -o iobound iobound.c

cpubound: cpubound.c
	gcc $(CFLAGS) -o cpubound cpubound.c

clean:
	rm -f $(OBJECTS)