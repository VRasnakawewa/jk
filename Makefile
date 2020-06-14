CC=gcc
CFLAG=-Wall -g
LIBS=-lssl -lcrypto

all: jk

jk: jk.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o util.o info.o common.h stack.h
	$(CC) $(CFLAG) -o jk jk.o ben.o reap.o ev.o map.o list.o jstr.o urlencode.o util.o info.o $(LIBS)

jstr.o: jstr.h
list.o: list.h
map.o: map.h
ev.o: ev.h
reap.o: reap.h
ben.o: ben.h
urlencode.o: urlencode.h
util.o: util.h
info.o: info.h

clean:
	rm -f jk *.o
