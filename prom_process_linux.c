// Phil Budne
// 2020-04-11
//
// prometheus process vars for Linux
// (pretty much) just the standard values defined at:
// https://prometheus.io/docs/instrumenting/writing_clientlibs/#process-metrics

/*-
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2020, Philip L. Budne
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <sys/types.h>			/* pid_t */
#include <sys/resource.h>		/* getrlimit */

#include <string.h>			/* strncmp */
#include <stdlib.h>			/* atol() */
#include <time.h>			/* time(), time_t */
#include <unistd.h>			/* getpid(), sysconf() */

#include "prom.h"
#include "common.h"

////////////////

// one time constants:
static long tix;			/* ticks per second */
static long pagesize;			/* bytes/page */

// data from /proc/PID/stat
// from proc(5) man page

// table defining macro, expanded multiple times
// with different definitions of PROC_VAR & PROC_BUF
// this sort of thing was the norm in PDP-10 assembler.
#define PROC_VARS \
    PROC_INT(pid) \
    PROC_BUF(comm) \
    PROC_VAR(state, char, "%c") \
    PROC_INT(ppid) \
    PROC_INT(pgrp) \
    PROC_INT(sess) \
    PROC_INT(tty_nr) \
    PROC_INT(tpgid) \
    PROC_UINT(flags) \
    PROC_ULONG(minflt) \
    PROC_ULONG(cminflt) \
    PROC_ULONG(majflt) \
    PROC_ULONG(cmajflt) \
    PROC_ULONG(utime) \
    PROC_ULONG(stime) \
    PROC_LONG(cutime) \
    PROC_LONG(cstime) \
    PROC_LONG(prio) \
    PROC_LONG(nice) \
    PROC_LONG(threads) \
    PROC_LONG(itreal) \
    PROC_VAR(start, long long, "%lld") \
    PROC_ULONG(vsize) \
    PROC_LONG(rss)

//  PROC_ULONG(rsslim)

// shorthands
#define PROC_INT(VAR) PROC_VAR(VAR, int, "%d")
#define PROC_UINT(VAR) PROC_VAR(VAR, unsigned, "%u")
#define PROC_LONG(VAR) PROC_VAR(VAR, long, "%ld")
#define PROC_ULONG(VAR) PROC_VAR(VAR, unsigned long, "%lu")

// declare a static struct to hold data from /proc
// (so it can be shared between getters)
static struct {
#define PROC_VAR(NAME, TYPE, FMT) TYPE NAME;
#define PROC_BUF(NAME) char NAME [64];
    PROC_VARS
#undef PROC_VAR
#undef PROC_BUF
} proc_stat;

static const char nvars[] = {
#define PROC_VAR(NAME, TYPE, FMT) 1,
#define PROC_BUF(NAME) 1,
    PROC_VARS
#undef PROC_VAR
#undef PROC_BUF
};
#define NUM_PROC_STAT sizeof(nvars)

static int
_read_proc(void) {
    FILE *f;
    unsigned n;
    static time_t last_proc;
    static char proc_stat_file[32];

    if (!STALE(last_proc))
	return 0;

    if (!pagesize) {		/* once only */
	snprintf(proc_stat_file, sizeof(proc_stat_file), "/proc/%d/stat", getpid());
	tix = sysconf(_SC_CLK_TCK);
	pagesize = sysconf(_SC_PAGESIZE);
    }

    f = fopen(proc_stat_file, "r");
    if (!f)
	return -1;

    // ORDINARILY fscanf is a mistake (win/lose)!
    n = fscanf(f,
#define PROC_VAR(NAME, TYPE, FMT) FMT " "
#define PROC_BUF(NAME) "%s "
	   PROC_VARS		// expand formats
#undef PROC_VAR
#undef PROC_BUF
#define PROC_VAR(NAME, TYPE, FMT) ,&proc_stat.NAME
#define PROC_BUF(NAME) ,proc_stat.NAME
	   PROC_VARS		// expand ptrs to vars
#undef PROC_VAR
#undef PROC_BUF
	   );
    fclose(f);
    if (n < NUM_PROC_STAT)
	return -1;		// will retry

    last_proc = prom_now;
    return 0;
}

static int
read_proc(void) {
    DECLARE_LOCK(read_proc_lock);
    int ret;

    LOCK(read_proc_lock);
    ret = _read_proc();
    UNLOCK(read_proc_lock);
    return ret;
}

////////////////////////////////////////////////////////////////
// implement variables

PROM_GETTER_COUNTER(process_cpu_seconds_total,
		    "Total user and system CPU time spent in seconds");

PROM_GETTER_COUNTER_FN_PROTO(process_cpu_seconds_total) {
    (void) pvp;
    if (read_proc() < 0)
	return 0;
    
    return (((double)proc_stat.utime)/tix +
	    ((double)proc_stat.stime)/tix);
}

////////////////
PROM_GETTER_GAUGE(process_virtual_memory_bytes,
		  "Virtual memory size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_bytes) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;
    return (double)proc_stat.vsize;
}

////////////////
#if 0
PROM_GETTER_GAUGE(process_virtual_memory_max_bytes,
		  "Maximum amount of virtual memory available in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_max_bytes) {
    (void) pvp;
    // not avail from getrlimit. need to read /proc/NN/limits?
    return 0.0;
}
#endif

////////////////
PROM_GETTER_GAUGE(process_resident_memory_bytes,
		  "Resident memory size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_resident_memory_bytes) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;
    return ((double)proc_stat.rss) * pagesize;
}

////////////////
// not even implemented by Java client library!!
#if 0			      // off: no good deed goes unpunished!
#include <malloc.h>	      // mallinfo

PROM_GETTER_GAUGE(process_heap_bytes,"Process heap size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_heap_bytes) {
    struct mallinfo mi = mallinfo();
    (void) pvp;
    return (double)mi.uordblks;
}
#endif

////////////////
// not in the process_ namespace

PROM_GETTER_GAUGE(num_threads,
		  "Number of process threads");

PROM_GETTER_GAUGE_FN_PROTO(num_threads) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;

    return proc_stat.threads;
}

////////////////

// no-op call this to force loading this file
int
prom_process_init(void) {
    return prom_process_common_init();
}
