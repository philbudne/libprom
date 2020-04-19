# requires gnu make

OS := $(shell uname)

CFLAGS=-O -g -Wextra -Wall -I.

ALL=libprom.a test_http
all:	$(ALL)

LIBOBJS=prom.o prom_http.o prom_process.o

ifeq ($(OS), Linux)
LIBOBJS += prom_process_linux.o
endif

ifeq ($(OS), FreeBSD)
LIBOBJS += prom_process_fbsd.o
endif

ifeq ($(OS), Darwin)
LIBOBJS += prom_process_osx.o
endif

$(LIBOBJS): prom.h

libprom.a: $(LIBOBJS)
	ar rc libprom.a $(LIBOBJS)

TEST=tests/001_test_http.c libprom.a
test_http: $(TEST)
	$(CC) $(CFLAGS) -o test_http $(TEST) $(TESTLIBS)

clean:
	rm -f $(ALL) $(LIBOBJS) *~
