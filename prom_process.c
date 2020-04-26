// Phil Budne
// 2020-04-18
//
// prometheus process vars common on Un*xy systems
// (FreeBSD, macOS, Linux)

#include <sys/time.h>			/* gettimeofday */
#include <sys/resource.h>		/* getrlimit */

#include <dirent.h>			/* opendir... */

#include "prom.h"
#include "common.h"

////////////////
// a gauge in all implementations I've looked at:
PROM_SIMPLE_GAUGE(process_start_time_seconds,
		  "Start time of the process in seconds since Unix epoch");

// run as a constructor:
static void get_start_time(void) __attribute((constructor));

static void
get_start_time(void) {
    // "SIMPLE" variables are integer-only
    // so no point in calling gettimeofday()
    PROM_SIMPLE_GAUGE_SET(process_start_time_seconds, time(0));
}

////////////////
PROM_GETTER_GAUGE(process_open_fds,
		  "Number of open file descriptors");

PROM_GETTER_GAUGE_FN_PROTO(process_open_fds) {
    DIR *d;
    static unsigned fds;
    DECLARE_LOCK(fds_lock);
    static time_t last_fds;

    LOCK(fds_lock);
    if (STALE(last_fds) &&
	(d = opendir("/dev/fd"))) {
	struct dirent *dp;

	fds = 0;
	// readdir_r is deprecated
	while ((dp = readdir(d)))
	    if (dp->d_name[0] != '.')
		fds++;
	closedir(d);
#ifndef __FreeBSD__		// ?!
	fds--;			// opendir fd
#endif
    }
    UNLOCK(fds_lock);

    return fds;
}

////////////////
PROM_GETTER_GAUGE(process_max_fds,
		  "Maximum number of open file descriptors");

PROM_GETTER_GAUGE_FN_PROTO(process_max_fds) {
    static struct rlimit maxfds;

    if (!maxfds.rlim_cur)
	getrlimit(RLIMIT_NOFILE, &maxfds);

    return maxfds.rlim_cur;
}

////////////////
// Of dubious value : make optional?

// Linux man page says:
// "Since the value is a long, on machines with a 32-bit long either
// this limit is at most 2 GiB, or this resource is unlimited."

PROM_GETTER_GAUGE(process_virtual_memory_max_bytes,
		  "Maximum amount of virtual memory available in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_max_bytes) {
    static struct rlimit maxvsz;

    if (!maxvsz.rlim_cur)
	getrlimit(RLIMIT_AS, &maxvsz);

    return maxvsz.rlim_cur;
}

////////////////
#ifdef USE_GETRUSAGE
PROM_GETTER_COUNTER(process_cpu_seconds_total,
		    "Total user and system CPU time spent in seconds");

PROM_GETTER_COUNTER_FN_PROTO(process_cpu_seconds_total) {
    struct rusage ru;

    if (getrusage(RUSAGE_SELF, &ru) < 0)
	return -1;
    return ((ru.ru_utime.tv_sec  + ru.ru_stime.tv_sec) +
	    (ru.ru_utime.tv_usec + ru.ru_stime.tv_usec) / 1000000.0);
}
#endif

////////////////

// called from prom_process_init() to force loading
int
prom_process_common_init(void) {
    return 0;
}
