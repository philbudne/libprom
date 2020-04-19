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
#include <sys/resource.h>		/* getrlimit */
#include <sys/sysctl.h>			/* KERN_PROC_PID */
#include <sys/user.h>			/* kinfo_proc */

#include <dirent.h>			/* opendir... */
#include <string.h>			/* strncmp */
#include <stdlib.h>			/* atol() */
#include <time.h>			/* time(), time_t */
#include <unistd.h>			/* getpid() */

#ifndef NO_THREADS
#include <pthread.h>

#define MUTEX_INIT PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP

#define DECLARE_LOCK(NAME) \
    static pthread_mutex_t NAME = MUTEX_INIT;

#define LOCK(NAME) pthread_mutex_lock(&NAME)
#define UNLOCK(NAME) pthread_mutex_unlock(&NAME)

#else
#define DECLARE_LOCK(NAME)
#define LOCK(NAME)
#define UNLOCK(NAME)
#endif

#include "prom.h"

////////////////

static time_t last_proc;
static struct rlimit maxfds, maxvsz;
static double rss, vsz, start, seconds;
static int threads, fds;
static int pagesize;

static int
_read_proc(void) {
    DIR *d;
    size_t length;
    struct kinfo_proc ki;
    static int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0 };

    if (!STALE(last_proc))
	return 0;

    if (!pagesize) {
	mib[3] = getpid();
	getrlimit(RLIMIT_NOFILE, &maxfds);
	getrlimit(RLIMIT_AS, &maxvsz);
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
    start = (ki.ki_start.tv_sec +
	     ki.ki_start.tv_usec / 1000000.0);
    rss = ki.ki_rssize * pagesize;
    vsz = ki.ki_size;		// seems like bytes!
    threads = ki.ki_numthreads;

    fds = 0;
    if ((d = opendir("/dev/fd"))) {
	struct dirent *dp;
	while ((dp = readdir(d))) {
	    if (dp->d_name[0] != '.')
		fds++;
	}
	// no open fd to account for???
	closedir(d);
    }

    last_proc = prom_now;
    return 0;
}

static int
read_proc(void) {
    int ret;
    DECLARE_LOCK(read_proc_lock);

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
PROM_GETTER_GAUGE(process_open_fds,
		  "Number of open file descriptors");

PROM_GETTER_GAUGE_FN_PROTO(process_open_fds) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;

    return fds;
}

////////////////
PROM_GETTER_GAUGE(process_max_fds,
		  "Maximum number of open file descriptors");

PROM_GETTER_GAUGE_FN_PROTO(process_max_fds) {
    (void) pvp;
    return maxfds.rlim_cur;
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
PROM_GETTER_GAUGE(process_virtual_memory_max_bytes,
		  "Maximum amount of virtual memory available in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_max_bytes) {
    (void) pvp;
    return maxvsz.rlim_cur;
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
PROM_GETTER_GAUGE(process_start_time_seconds,
		  "Start time of the process since unix epoch in seconds");

PROM_GETTER_GAUGE_FN_PROTO(process_start_time_seconds) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;

    return start;
}

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
    return 0;
}
