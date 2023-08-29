.POSIX:
.PHONY: all clean install uninstall dist

include config.mk

all: xorblednam

xorblednam: xorblednam.o
	$(CC) $(LDFLAGS) -o xorblednam xorblednam.o $(LDLIBS)

clean:
	rm -f xorblednam xorblednam.o xorblednam-$(VERSION).tar.gz

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f xorblednam $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/xorblednam
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	cp -f xorblednam.1 $(DESTDIR)$(MANPREFIX)/man1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/xorblednam.1

dist: clean
	mkdir -p xorblednam-$(VERSION)
	cp -R COPYING config.mk Makefile README xorblednam.1 \
		xorblednam.c xorblednam-$(VERSION)
	tar -cf xorblednam-$(VERSION).tar xorblednam-$(VERSION)
	gzip xorblednam-$(VERSION).tar
	rm -rf xorblednam-$(VERSION)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/xorblednam
	rm -f $(DESTDIR)$(MANPREFIX)/man1/xorblednam.1
