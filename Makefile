# See LICENSE file for copyright and license details.
VERSION = 0.1

.POSIX:

# Install paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Uncomment in OpenBSD
#MANPREFIX = ${PREFIX}/man

CC = clang

# Comment this line in OpenBSD
CFLAGS   += -fsanitize=address,undefined

CFLAGS   += -Wall -Wextra -Wpedantic -std=gnu11
CFLAGS   += -Wconversion -Wdouble-promotion
CFLAGS   += -fpie -Wstack-protector --param ssp-buffer-size=4
CFLAGS   += -pedantic -fstack-protector-all -fstack-protector-strong
CFLAGS   +=  -fpic -Wstack-protector
CFLAGS   += -Werror=format-security -Werror=implicit-function-declaration
CFLAGS   += -pedantic-errors -Wunused-result -Wchar-subscripts
CFLAGS   += -Wformat-security -fstack-clash-protection -Wdouble-promotion
CFLAGS   += -Wimplicit-fallthrough -Wignored-qualifiers
CFLAGS   += -Wmisleading-indentation -Wswitch -Wunused-parameter -ftrapv 
CFLAGS   += -Wunused-variable -Wuninitialized
CFLAGS   += -Walloca -Warray-bounds -Wsign-compare
CFLAGS   += -Wshadow -Wpointer-arith -Wcomments -Wwrite-strings
CFLAGS   += -Wcast-align -Wdangling-else -Wenum-compare -Wenum-conversion
CFLAGS   += -Wsign-conversion -Wvla

CFLAGS   += -Wno-unused-command-line-argument

# Libmagic dependency (uses pkg-config, feel free to modify if needed)
LDFLAGS  += $(shell pkg-config --cflags libmagic)
CFLAGS   += $(shell pkg-config --libs libmagic)
#LDFLAGS  += `pkg-config --cflags libmagic`
#CFLAGS   += `pkg-config --libs libmagic`

CPPFLAGS += -D_FORTIFY_SOURCE=2

# Utilities
RM := rm -rf

BIN  = opn
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
