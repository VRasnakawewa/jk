CC=gcc
CFLAG=-Wall -g

all: jk

jk: jk.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o common.h
	$(CC) -o jk jk.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o

jstr.o: jstr.h
list.o: list.h
map.o: map.h
ev.o: ev.h
reap.o: reap.h
ben.o: ben.h
urlencode.o: urlencode.h

clean:
	rm -f jk *.o
