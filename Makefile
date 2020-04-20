# requires gnu make

OS := $(shell uname)

CFLAGS=-O -g -Wextra -Wall -Wmissing-prototypes

TESTS=test_http test_hist

ALL=libprom.a
all:	$(ALL)

test_progs: $(TESTS)

LIBOBJS=prom.o prom_http.o prom_process.o prom_histogram.o

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

libprom.a: $(LIBOBJS)
	ar rc libprom.a $(LIBOBJS)

$(LIBOBJS): prom.h
prom_process.o prom_process_fbsd.o prom_process_linux.o prom_process_osx.o: common.h

################
TEST_CFLAGS=$(CFLAGS) -I.

TEST_HTTP=tests/001_test_http.c libprom.a
test_http: $(TEST_HTTP)
	$(CC) $(TEST_CFLAGS) -o test_http $(TEST_HTTP) $(TESTLIBS)

TEST_HIST=tests/002_hist.c libprom.a
test_hist: $(TEST_HIST)
	$(CC) $(TEST_CFLAGS) -o test_hist $(TEST_HIST) $(TESTLIBS)


################
clean:
	rm -f $(ALL) $(LIBOBJS) $(TESTS) *~
