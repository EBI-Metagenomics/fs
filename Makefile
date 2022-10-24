.POSIX:

FS_VERSION := 1.3.0

CC ?= gcc
CFLAGS := $(CFLAGS) -Wall -Wextra

SRC := fs.c
OBJ := $(SRC:.c=.o)
HDR := fs.h

all: tests

%.o: %.c
	$(CC) $(CFLAGS) -c $<

tests.o: tests.c $(HDR)
	$(CC) $(CFLAGS) -c $<

tests: tests.o $(OBJ)
	$(CC) -o $@ $^

test check: tests
	./tests

dist: clean
	mkdir -p fs-$(FS_VERSION)
	cp -R README.md LICENSE $(SRC) $(HDR) fs-$(FS_VERSION)
	tar -cf - fs-$(FS_VERSION) | gzip > fs-$(FS_VERSION).tar.gz
	rm -rf fs-$(FS_VERSION)

distclean:
	rm -f fs-$(FS_VERSION).tar.gz

clean: distclean
	rm -f tests *.o

.PHONY: all check test dist distclean clean
