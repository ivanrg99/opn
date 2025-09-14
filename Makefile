# See LICENSE file for copyright and license details.
VERSION = 0.1

.POSIX:

# Install paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Uncomment in OpenBSD
#MANPREFIX = ${PREFIX}/man

CC = clang

CFLAGS   += -Wall -Wextra -Wpedantic -std=gnu11 

# Libmagic dependency (uses pkg-config, feel free to modify if needed)
CFLAGS  += `pkg-config --cflags libmagic`
LDFLAGS += `pkg-config --libs libmagic`

# Utilities
RM := rm -rf

BIN = opn
SRC = opn.c
OBJ = ${SRC:.c=.o}

# Targets
all: $(BIN)

$(BIN) : $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@

clean:
	$(RM) $(OBJ) $(BIN)

install: release
	strip $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(BIN)
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < $(BIN).1 > $(DESTDIR)$(MANPREFIX)/man1/$(BIN).1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/$(BIN).1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN) $(DESTDIR)$(MANDIR)/$(MAN)

debug: CFLAGS += -DDEBUG -g3
debug: $(BIN)

release: CFLAGS += -O3 -DNDEBUG
release: clean
release: $(BIN)

.PHONY: clean
