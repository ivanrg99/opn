# See LICENSE file for copyright and license details.
VERSION = 0.1

.POSIX:

# Install paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

# Uncomment in OpenBSD
#MANPREFIX = ${PREFIX}/man

CC = gcc

CFLAGS   += -Wall -Wextra -Wpedantic -std=gnu11
CFLAGS   += -fsanitize=address,undefined -Wconversion -Wdouble-promotion
CFLAGS   += -fpie -Wl,-pie -Wstack-protector --param ssp-buffer-size=4
CFLAGS   += -pedantic -fstack-protector-all -fstack-protector-strong
CFLAGS   +=  -fpic -D_FORTIFY_SOURCE=2 -Wzero-length-bounds -Wtrampolines
CFLAGS   += -Werror=format-security -Werror=implicit-function-declaration -Wstack-protector
CFLAGS   += -pedantic-errors -Wunused-result -Wchar-subscripts -Wdouble-promotion
CFLAGS   += -Wformat-security -Wformat-signedness -Wformat-overflow -fstack-clash-protection
CFLAGS   += -Wimplicit-fallthrough -Wno-if-not-aligned -Wignored-qualifiers -Wl,-z,noexecstack
CFLAGS   += -Wmisleading-indentation -Wswitch -Wunused-parameter -ftrapv 
CFLAGS   += -Wunused-variable -Wuninitialized -Wmaybe-uninitialized -Wstrict-overflow=5
CFLAGS   += -Wstringop-overflow=4 -Walloca -Warray-bounds -Wl,-z,relro,-z,now
CFLAGS   += -Wbool-compare -Wduplicated-branches -Wduplicated-cond
CFLAGS   += -Wshadow -Wunsafe-loop-optimizations -Wpointer-arith -Wcomments -Wwrite-strings
CFLAGS   += -Wcast-align -Wdangling-else -Wenum-compare -Wenum-conversion -Wsign-compare
CFLAGS   += -Wsign-conversion -Wlogical-op -Wvla -Wvector-operation-performance

# Libmagic dependency (uses pkg-config, feel free to modify if needed)
LDFLAGS  += `pkg-config --cflags libmagic`
CPPFLAGS += `pkg-config --libs libmagic`

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
