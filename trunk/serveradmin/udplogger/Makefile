REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

CC=gcc
CFLAGS=-pedantic-errors -Wall -DREVISION=${REVISION} -D__DEBUG__=1
LDLIBS=-lz -pthread


all: udplogger udploggercat

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udplogger
	rm -f udploggercat

rebuild: realclean all

udplogger: beacon.o socket.o trim.o udplogger.o

udploggercat: udploggercat.o
