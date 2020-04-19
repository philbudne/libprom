// Phil Budne
// 2020-04-17
//
// prometheus process vars for FreeBSD
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
#include <sys/sysctl.h>			/* KERN_PROC_PID */
#include <sys/user.h>			/* kinfo_proc */

#include <string.h>			/* strncmp */
#include <time.h>			/* time(), time_t */
#include <unistd.h>			/* getpid() */

#include "prom.h"
#include "common.h"

////////////////

static double rss, vsz, seconds;
static int threads;
static int pagesize;

// helper, called under lock
static int
_read_proc(void) {
    size_t length;
    struct kinfo_proc ki;
    static int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0 };
    static time_t last_proc;

    if (!STALE(last_proc))
	return 0;

    if (!pagesize) {
	mib[3] = getpid();
	pagesize = getpagesize();
    }

    length = sizeof(ki);
    if (sysctl(mib, 4, &ki, &length, NULL, 0) < 0) {
	perror("sysctl");
	return -1;
    }
    seconds = ((ki.ki_rusage.ru_utime.tv_sec  +
		ki.ki_rusage.ru_stime.tv_sec) +
	       (ki.ki_rusage.ru_utime.tv_usec +
		ki.ki_rusage.ru_stime.tv_usec) / 1000000.0);
    rss = ki.ki_rssize * pagesize;
    vsz = ki.ki_size;		// seems like bytes!
    threads = ki.ki_numthreads;

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
	return 0.0;

    return seconds;
}

////////////////
PROM_GETTER_GAUGE(process_virtual_memory_bytes,
		  "Virtual memory size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_bytes) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;
    return vsz;
}

////////////////
PROM_GETTER_GAUGE(process_resident_memory_bytes,
		  "Resident memory size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_resident_memory_bytes) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;
    return rss;				/* XXX */
}

////////////////
// not even implemented by Java client library!!
#if 0			      // off: no good deed goes unpunished!

PROM_GETTER_GAUGE(process_heap_bytes,"Process heap size in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_heap_bytes) {
    (void) pvp;
    // mallctl stats.allocated??
    return -1;
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

    return threads;
}

////////////////

// no-op call this to force loading this file
int
prom_process_init(void) {
    return prom_process_common_init();
}
