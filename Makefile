# requires gnu make

OS := $(shell uname)

HISTOGRAMS = 1

CFLAGS=-O -g -Wextra -Wall -Wmissing-prototypes -I.

ALL=libprom.a test_http
all:	$(ALL)

LIBOBJS=prom.o prom_http.o prom_process.o

ifeq ($(OS), Linux)
LIBOBJS += prom_process_linux.o
# optional (better resolution); set to zero to disable
USE_GETRUSAGE = 1
endif

ifeq ($(OS), FreeBSD)
LIBOBJS += prom_process_fbsd.o
endif

ifeq ($(OS), Darwin)
LIBOBJS += prom_process_osx.o
USE_GETRUSAGE = 1
endif

ifeq ($(USE_GETRUSAGE), 1)
CFLAGS += -DUSE_GETRUSAGE
endif

ifneq ($(HISTOGRAMS), 0)
LIBOBJS += prom_histogram.o
CFLAGS += -DPROM_HISTOGRAMS
endif

$(LIBOBJS): prom.h

libprom.a: $(LIBOBJS)
	ar rc libprom.a $(LIBOBJS)

TEST=tests/001_test_http.c libprom.a
test_http: $(TEST)
	$(CC) $(CFLAGS) -o test_http $(TEST) $(TESTLIBS)

clean:
	rm -f $(ALL) $(LIBOBJS) *~
