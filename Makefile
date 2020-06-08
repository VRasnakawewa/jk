CC=gcc
CFLAG=-Wall -g

all: jk

jk: jk.o reap.o ev.o map.o list.o jstr.o common.h
	$(CC) -o jk jk.o reap.o ev.o map.o list.o jstr.o

jstr.o: jstr.h
list.o: list.h
map.o: map.h
ev.o: ev.h
reap.o: reap.h

clean:
	rm -f jk *.o
