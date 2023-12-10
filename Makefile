# PROJECT INFO HERE
# See LICENSE file for copyright and license details.

.POSIX:

# Install paths
PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man1

# Compiler flags (much more for GCC)
# A lot of hardening an extra warnings are included
# If compiling with another compiler, adjust as needed
CC = gcc

# If clang (more to come):
#CFLAGS   += -g3 -Wall -Wextra -Wpedantic -Werror -std=c99
#CFLAGS   += -fsanitize=address,undefined -Wconversion -Wdouble-promotion
#CFLAGS   += -fpie -fsanitize-trap=all  -Wstack-protector --param ssp-buffer-size=4
#CFLAGS   += -pedantic -fstack-protector-all -fstack-protector-strong
#CFLAGS   +=  -fpic -fcf-protection 
#CFLAGS   += -Werror=format-security -Werror=implicit-function-declaration -Wstack-protector
#CFLAGS   += -pedantic-errors -Wunused-result -Wchar-subscripts -Wdouble-promotion
#CFLAGS   += -Wformat-security -fstack-clash-protection
#CFLAGS   += -Wimplicit-fallthrough -Wignored-qualifiers 
#CFLAGS   += -Wmisleading-indentation -Wswitch -Wunused-parameter -ftrapv 
#CFLAGS   += -Wunused-variable -Wuninitialized -Wstrict-overflow=5
#CFLAGS   += -Walloca -Warray-bounds 
#CFLAGS   += -Wshadow -Wpointer-arith -Wcomments -Wwrite-strings
#CFLAGS   += -Wcast-align -Wdangling-else -Wenum-compare -Wenum-conversion -Wsign-compare
#CFLAGS   += -Wsign-conversion -Wvla

# If gcc:
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


# Main folder variables
BIN  = opn
MAN  = $(BIN).1
SRC  = src
OBJ  = obj
TEST = tests

# Target folders
SRCS      = $(wildcard $(SRC)/*.c)
OBJS      = $(patsubst $(SRC)/%.c, $(OBJ)/%.o, $(SRCS))

TESTS     = $(wildcard $(TEST)/*.c)
TEST_BINS = $(patsubst $(TEST)/%.c, $(TEST)/bin/%, $(TESTS))

MAIN_OBJ  = $(OBJ)/main.o
TEST_OBJS = $(filter-out $(MAIN_OBJ), $(OBJS))

# Utilities
RM := rm -rf
DIR_CREATE = mkdir -p $(@D)

# Targets

all: $(BIN)

$(BIN) : $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ -o $@
	$(info CREATED $@)


$(OBJ)/%.o: $(SRC)/%.c
	@$(DIR_CREATE)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)  -c $< -o $@
	$(info CREATED OBJ $@)

clean:
	$(RM) $(OBJ) $(TEST)/bin $(BIN)

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
release: test

test: $(TEST)/bin $(TEST_BINS)
	for test in $(TEST_BINS); do ./$$test; done

$(TEST)/bin: clean $(TEST_OBJS)
	mkdir $@

$(TEST)/bin/%: $(TEST)/%.c
	$(CC) $(CFLAGS) $^ $(TEST_OBJS) $(LDFLAGS) -o $@

run: $(BIN)
	./$(BIN) test_file -d code

.PHONY: test clean

