REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

CC=gcc
CFLAGS=-pedantic-errors -Wall -DREVISION=${REVISION}
#CFLAGS+=-D__DEBUG__

all: udploggerd udploggerc

install: all
	mkdir -v -p "/usr/local/stow/udplogger-r${REVISION}/sbin"
	cp udploggerc udploggerd "/usr/local/stow/udplogger-r${REVISION}/sbin"

clean:
	rm -f *.o

pristine: realclean

proper: realclean

realclean: clean
	rm -f udploggerc
	rm -f udploggerd
	rm -f udplogger-r*.tar.gz

rebuild: realclean all

source-package:
	mkdir "udplogger-r${REVISION}"
	cp *.c *.h Makefile "udplogger-r${REVISION}"
	perl -i -p -e "s/^REVISION=.*/REVISION=${REVISION}/" "udplogger-r${REVISION}/Makefile"
	tar cfz "udplogger-r${REVISION}.tar.gz" "udplogger-r${REVISION}"
	rm -rf "udplogger-r${REVISION}"

udploggerc: udploggerclientlib.o udploggerc.o socket.o
	${CC}   ${^} ${LDLIBS} -o ${@}

udploggerd: beacon.o socket.o trim.o udploggerd.o
	${CC}   ${^} ${LDLIBS} -pthread -o ${@}
