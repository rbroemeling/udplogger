REVISION=${shell svn info . | grep '^Revision: ' | sed -e 's/[^0-9]//g'}

all:

install:
	mkdir -v -p "/usr/local/stow/udploggertools-r${REVISION}/sbin"
	cp *.py "/usr/local/stow/udploggertools-r${REVISION}/sbin"

clean:
	find . -iname '*.pyc' -delete

pristine: realclean

proper: realclean

realclean: clean
	rm -f udploggertools-r*.tar.gz

source-package:
	mkdir "udploggertools-r${REVISION}"
	cp *.py Makefile "udploggertools-r${REVISION}"
	perl -i -p -e "s/^REVISION=.*/REVISION=${REVISION}/" "udploggertools-r${REVISION}/Makefile"
	tar cfz "udploggertools-r${REVISION}.tar.gz" "udploggertools-r${REVISION}"
	rm -rf "udploggertools-r${REVISION}"
