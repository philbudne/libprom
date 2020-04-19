// Phil Budne
// 2020-04-17
//
// prometheus process vars for OSX
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
#include <sys/resource.h>		/* getrlimit, getrusage */

#include <mach/task.h>
#include <mach/mach_init.h>

#include <string.h>			/* strncmp */
#include <time.h>			/* time(), time_t */
#include <unistd.h>			/* getpid() */

#ifndef NO_THREADS
#include <pthread.h>

#define MUTEX_INIT PTHREAD_MUTEX_INITIALIZER

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

static struct rlimit maxvsz;
static double rss, vsz, seconds;

static int
_read_proc(void) {
    DIR *d;
    struct rusage ru;
    static task_t task;
    static time_t last_proc;
    mach_task_basic_info_data_t mtbi;
    mach_msg_type_number_t mtbic;
    
    if (!STALE(last_proc))
	return 0;

    if (!maxfds.rlim_cur) {
	if (task_for_pid(current_task(), getpid(), &task) != KERN_SUCCESS)
	    return -1;
	getrlimit(RLIMIT_AS, &maxvsz);
    }

    // thanks to miknight.blogspot.com/2005/11/resident-set-size-in-mac-os-x.html for a start!
    mtbic = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(task, MACH_TASK_BASIC_INFO, (task_info_t)&mtbi, &mtbic) != KERN_SUCCESS)
	return -1;
    rss = mtbi.resident_size;
    vsz = mtbi.virtual_size;

    if (getrusage(RUSAGE_SELF, &ru) < 0)
	return -1;
    seconds = ((ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
	       (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000000.0);

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
    if (read_proc() < 0)
	return 0.0;
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
// not in the process_ namespace
#if 0
PROM_GETTER_GAUGE(num_threads,
		  "Number of process threads");

PROM_GETTER_GAUGE_FN_PROTO(num_threads) {
    (void) pvp;
    if (read_proc() < 0)
	return 0.0;

    return threads;
}
#endif

////////////////

// no-op call this to force loading this file
int
prom_process_init(void) {
    return prom_process_common_init();
}

