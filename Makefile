# PROJECT INFO HERE
# See LICENSE file for copyright and license details.

.POSIX:

# Install paths
PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man1

CC = gcc

CFLAGS   += -g3 -Wall -Wextra -Wpedantic -std=gnu11
CFLAGS   += -fsanitize=address,undefined -Wconversion -Wdouble-promotion
CFLAGS   += -fpie -Wl,-pie -Wstack-protector --param ssp-buffer-size=4
CFLAGS   += -pedantic -fstack-protector-all -fstack-protector-strong
CFLAGS   +=  -fpic 
CFLAGS   += -Werror=format-security -Werror=implicit-function-declaration -Wstack-protector
CFLAGS   += -pedantic-errors -Wunused-result -Wchar-subscripts -Wdouble-promotion
CFLAGS   += -Wformat-security -Wformat-signedness -Wformat-overflow -fstack-clash-protection
CFLAGS   += -Wimplicit-fallthrough -Wno-if-not-aligned -Wignored-qualifiers -Wl,-z,noexecstack
CFLAGS   += -Wmisleading-indentation -Wswitch -Wunused-parameter -ftrapv 
CFLAGS   += -Wunused-variable -Wuninitialized -Wmaybe-uninitialized -Wstrict-overflow=5
CFLAGS   += -Wstringop-overflow=4 -Walloca -Warray-bounds -Wl,-z,relro,-z,now
CFLAGS   += -Wbool-compare -Wduplicated-branches -Wduplicated-cond -Wzero-length-bounds -Wtrampolines
CFLAGS   += -Wshadow -Wunsafe-loop-optimizations -Wpointer-arith -Wcomments -Wwrite-strings
CFLAGS   += -Wcast-align -Wdangling-else -Wenum-compare -Wenum-conversion -Wsign-compare
CFLAGS   += -Wsign-conversion -Wlogical-op -Wvla -Wvector-operation-performance

CPPFLAGS += -DDEBUG -D_FORTIFY_SOURCE=2
LDFLAGS += -lmagic

# Utilities
RM := rm -rf

BIN  = opn
SRC = main.c
OBJ = ${SRC:.c=.o}

# Targets
all: $(BIN)

$(BIN) : $(OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@


clean:
	$(RM) $(OBJ) $(BIN)

install: $(BIN)
	mkdir -p $(DESTDIR)$(PREFIX)/bin/
	cp -f $(BIN) $(DESTDIR)$(PREFIX)/bin/
	chmod 555 $(DESTDIR)$(PREFIX)/bin/$(BIN)
	mkdir -p $(DESTDIR)$(MANDIR)
	sed -e "s/%VERSION%/$(GETVER)/" $(MAN) > $(DESTDIR)$(MANDIR)/$(MAN)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(BIN) $(DESTDIR)$(MANDIR)/$(MAN)

release: CFLAGS=-Wall -Wextra -Os -DNDEBUG
release: clean
release: $(BIN)

.PHONY: clean

