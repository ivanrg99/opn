# PROJECT INFO HERE
# See LICENSE file for copyright and license details.

.POSIX:

# Install paths
PREFIX = /usr/local
MANDIR = $(PREFIX)/share/man/man1

# Compiler flags
CC        = clang
CFLAGS   += -g -Wall -Wextra -Wpedantic -fsanitize-trap -Werror -std=c99
CFLAGS   += -fsanitize=address,undefined -Wconversion -Wdouble-promotion
CPPFLAGS += -DDEBUG

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
	./$(BIN) test_file

.PHONY: test clean

