// simple/sample request dispatch using pthread pool
// Phil Budne 2020-04-21

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

#include <pthread.h>
#include <stdlib.h>			/* calloc */
#include <unistd.h>			/* close */
#include <fcntl.h>

#include "prom.h"

#define POOL_SIZE 3
#define QUEUE_SIZE 8

static int pool_size;
static pthread_t *pool_threads;
static pthread_mutex_t pool_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t pool_cv = PTHREAD_COND_INITIALIZER;
static const char *exporter_name;

static int qrd, qwr;
static int queue[QUEUE_SIZE];

PROM_SIMPLE_GAUGE(promhttp_metric_handler_requests_in_flight,
		  "Current number of scrapes being served");

#define NEXT(X) (((X)+1) % QUEUE_SIZE)
#define QEMPTY (qrd == qwr)
#define QFULL (NEXT(qwr) == qrd)

static void *
prom_pool_worker(void *arg) {
    (void) arg;
    for (;;) {
	int fd;
	FILE *f;

	pthread_mutex_lock(&pool_lock);
	if (QEMPTY)
	    pthread_cond_wait(&pool_cv, &pool_lock);
	// return if cond_wait returned non-zero?
	//printf("awake %#lx\n", pthread_self());
	if (QEMPTY) {
	    //puts("empty");
	    fd = -1;
	}
	else {
	    fd = queue[qrd];
	    qrd = NEXT(qrd);
	}
	pthread_mutex_unlock(&pool_lock);
	if (fd < 0)
	    continue;
	//printf("fd %d\n", fd);
	fcntl(fd, F_SETFD, 0);		/* clear nonblock */
	f = fdopen(fd, "r+");
	if (f) {
	    //sleep(1);
	    prom_http_request(f, f, exporter_name);
	    fclose(f);
	}
	else {
	    prom_http_unavail(fd);
	    close(fd);
	}
	PROM_SIMPLE_GAUGE_DEC(promhttp_metric_handler_requests_in_flight);
    }
    return NULL;
}

int
prom_pool_init(int threads, const char *name) {
    int i;

    pool_threads = calloc(threads, sizeof(pthread_t));

    for (i = 0; i < threads; i++)
	if (pthread_create(&pool_threads[i], NULL,
			   prom_pool_worker, NULL) == 0)
	    pool_size++;
	else
	    break;

    // XXX complain if pool_size < threads?
    exporter_name = name;
    return 0;
}

int
prom_dispatch(int s) {
    int ret;
    if (!exporter_name)
	return -1;
    pthread_mutex_lock(&pool_lock);
    if (!QFULL) {
	//printf("queue %d\n", s);
	PROM_SIMPLE_GAUGE_INC(promhttp_metric_handler_requests_in_flight);
	queue[qwr] = s;
	qwr = NEXT(qwr);
	pthread_cond_signal(&pool_cv);
	ret = 0;
    }
    else {
	prom_http_unavail(s);
	//puts("full");
	close(s);
	ret = -1;
    }
    pthread_mutex_unlock(&pool_lock);
    return ret;
}
