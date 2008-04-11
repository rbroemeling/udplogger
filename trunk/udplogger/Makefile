REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

CC=gcc
CFLAGS=-pedantic-errors -Wall -DREVISION=${REVISION} -D__DEBUG__
LDLIBS=-lz -pthread


all: udploggerd udploggercat

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udploggerd
	rm -f udploggercat

rebuild: realclean all

udploggerd: beacon.o socket.o trim.o udploggerd.o

udploggercat: udploggercat.o socket.o
