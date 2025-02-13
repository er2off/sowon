COMMON_CFLAGS=		-Wall -Wextra -std=c99 -pedantic
CFLAGS+=		`pkg-config --cflags sdl3` $(if $(PENGER),-DPENGER) $(COMMON_CFLAGS)
COMMON_LIBS=		-lm
LIBS=			`pkg-config --libs sdl3` $(COMMON_LIBS)
PREFIX?=		/usr/local
INSTALL?=		install

.PHONY: all
all: Makefile sowon man

sowon: main.c digits.h $(if $(PENGER),penger_walk_sheet.h)
	$(CC) $(CFLAGS) -o sowon main.c $(LIBS)

digits.h: png2c digits.png
	./png2c digits.png digits > digits.h

penger_walk_sheet.h: png2c penger_walk_sheet.png
	./png2c penger_walk_sheet.png penger > penger_walk_sheet.h

png2c: png2c.c
	$(CC) $(COMMON_CFLAGS) -o png2c png2c.c -lm

docs/sowon.6.gz: docs/sowon.6
	gzip -c docs/sowon.6 > docs/sowon.6.gz

.PHONY: man
man: docs/sowon.6.gz

.PHONY: clean
clean:
	rm sowon docs/sowon.6.gz png2c

.PHONY: install
install: all
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -C ./sowon $(DESTDIR)$(PREFIX)/bin
	$(INSTALL) -d $(DESTDIR)$(PREFIX)/man/man6
	$(INSTALL) -C docs/sowon.6.gz $(DESTDIR)$(PREFIX)/man/man6
