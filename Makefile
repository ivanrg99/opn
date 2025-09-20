.POSIX:

# See LICENSE file for copyright and license details.
VERSION = 0.1

# Install paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Uncomment in OpenBSD
#MANPREFIX = ${PREFIX}/man

CC = cc
PKG_CONFIG = pkg-config

# Main flags
CFLAGS += -Wall -Wextra -Wpedantic -std=gnu99

# Libmagic dependency
MAGIC_CFLAGS  = `$(PKG_CONFIG) --cflags libmagic`
MAGIC_LIBS    = `$(PKG_CONFIG) --libs libmagic`

CFLAGS  += $(MAGIC_CFLAGS)
LDFLAGS += $(MAGIC_LIBS)

BIN = opn
SRC = opn.c
OBJ = ${SRC:.c=.o}

# Targets
all: $(BIN)

$(BIN) : $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

clean:
	rm -f $(OBJ) $(BIN)

install: all
	strip $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(BIN)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < $(BIN).1 > $(DESTDIR)$(MANPREFIX)/man1/$(BIN).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(BIN).1

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/$(BIN) 
	rm $(DESTDIR)$(MANPREFIX)/man1/$(BIN).1


.PHONY: clean
