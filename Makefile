# This file is part of PODIFF.
# Copyright (C) 2011, 2012 Sergey Poznyakoff
#
# PODIFF is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# PODIFF is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this PODIFF.  If not, see <http://www.gnu.org/licenses/>.

PACKAGE=podiff
VERSION=1.1

PREFIX=/usr
BINDIR=$(PREFIX)/bin
MANDIR=$(PREFIX)/share/man

SOURCES=y.tab.c lex.yy.c main.c txtacc.c list.c
OBJECTS=$(SOURCES:.c=.o)
HEADERS=podiff.h
YACCSRC=podiff.y podiff.l
BUILT_FILES=lex.yy.c y.tab.c y.tab.h
MANFILES=podiff.1
EXTRA_DIST=COPYING README NEWS

CFLAGS=-ggdb
LIBS=

podiff: $(OBJECTS)
	cc $(CFLAGS) -opodiff $(OBJECTS) $(LIBS)

y.tab.c y.tab.h: podiff.y
	yacc -dt podiff.y

lex.yy.c: podiff.l
	flex podiff.l

$(OBJECTS): podiff.h

y.tab.o lex.yy.o: podiff.h

.c.o:
	cc -c $(CFLAGS) -DVERSION=\"$(VERSION)\" $<

clean:
	rm -f podiff *.o *~ $(PACKAGE)-$(VERSION)

allclean: clean
	rm -f $(BUILT_FILES)

LSFILES=find . -type f | sort 

distdir: $(BUILT_FILES)
	test -d $(PACKAGE)-$(VERSION) && rm -r $(PACKAGE)-$(VERSION); \
	mkdir $(PACKAGE)-$(VERSION); \
	cp $(SOURCES) $(YACCSRC) $(HEADERS) \
           $(BUILT_FILES) $(MANFILES) $(EXTRA_DIST) Makefile \
           $(PACKAGE)-$(VERSION); \
	cd $(PACKAGE)-$(VERSION); \
	touch .before .after; \
	$(LSFILES) > .before; \
	make; \
	make clean; \
	$(LSFILES) > .after; \
	files_left=`diff .before .after | sed -n 's/^> //p'`; \
	if test -n "$$files_left"; then \
		text="Files left after make clean:"; \
		echo $$text | sed 's/./=/g' >&2; \
		echo $$text >&2; \
		echo $$text | sed 's/./-/g' >&2; \
		echo $$files_left >&2; \
		echo $$text | sed 's/./=/g' >&2; \
		exit 1; \
	else \
		rm .before .after; \
	fi

dist: distdir
	tar cfhz $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)

install-bin: podiff
	cp podiff $(DESTDIR)$(BINDIR)

install-man: $(MANFILES)
	for file in $(MANFILES); \
	do \
		sec=`expr "$$file" : '.*\.\([1-8]\)'`; \
		cp $$file $(DESTDIR)$(MANDIR)/man$$sec; \
	done

install: install-bin install-man
