// Phil Budne
// 2020-04-11
//
// prometheus process vars common on Un*xy systems

#include <sys/time.h>			/* gettimeofday */
#include <sys/resource.h>		/* getrlimit */

#include <dirent.h>			/* opendir... */

#include "prom.h"
#include "common.h"

static void get_start_time(void) __attribute((constructor));

static double start;

static void
get_start_time(void) {
    struct timeval tv;

    gettimeofday(&tv, NULL);
    start = tv.tv_sec + tv.tv_usec / 1000000.0;
}

////////////////
PROM_GETTER_GAUGE(process_start_time_seconds,
		  "Start time of the process in seconds since Unix epoch");

PROM_GETTER_GAUGE_FN_PROTO(process_start_time_seconds) {
    (void) pvp;
    return start;
}

////////////////
PROM_GETTER_GAUGE(process_open_fds,
		  "Number of open file descriptors");

PROM_GETTER_GAUGE_FN_PROTO(process_open_fds) {
    DIR *d;
    static time_t last_fds;
    static unsigned fds;
    DECLARE_LOCK(fds_lock);

    (void) pvp;

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

    (void) pvp;
    if (!maxfds.rlim_cur)
	getrlimit(RLIMIT_NOFILE, &maxfds);

    return maxfds.rlim_cur;
}

////////////////
#ifndef __Linux__
// less useful on LP64 systems than on ILP32
PROM_GETTER_GAUGE(process_virtual_memory_max_bytes,
		  "Maximum amount of virtual memory available in bytes");

PROM_GETTER_GAUGE_FN_PROTO(process_virtual_memory_max_bytes) {
    static struct rlimit maxvsz;

    (void) pvp;
    if (!maxvsz.rlim_cur)
	getrlimit(RLIMIT_AS, &maxvsz);

    return maxvsz.rlim_cur;
}
#endif
////////////////

int
prom_process_common_init(void) {
    return 0;
}

