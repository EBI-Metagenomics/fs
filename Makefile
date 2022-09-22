.POSIX:

XFILE_VERSION := 1.1.0

CC ?= gcc
CFLAGS := $(CFLAGS) -Wall -Wextra

SRC := xfile.c
OBJ := $(SRC:.c=.o)
HDR := xfile.h

all: tests

%.o: %.c
	$(CC) $(CFLAGS) -c $<

tests.o: tests.c
	$(CC) $(CFLAGS) -c $<

tests: tests.o $(OBJ)
	$(CC) -o $@ $^

check: tests
	./tests

test: check

dist: clean
	mkdir -p xfile-$(XFILE_VERSION)
	cp -R README.md LICENSE $(SRC) $(HDR) xfile-$(XFILE_VERSION)
	tar -cf - xfile-$(XFILE_VERSION) | gzip > xfile-$(XFILE_VERSION).tar.gz
	rm -rf xfile-$(XFILE_VERSION)

distclean:
	rm -f xfile-$(XFILE_VERSION).tar.gz

clean: distclean
	rm -f tests *.o

.PHONY: all check test dist distclean clean
