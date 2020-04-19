// Phil Budne
// 2020-04-11
//
// prometheus common O/S defines
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

#ifndef NO_THREADS
#include <pthread.h>

#ifdef PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
#define MUTEX_INIT PTHREAD_ADAPTIVE_MUTEX_INITIALIZER_NP
#else
#define MUTEX_INIT PTHREAD_MUTEX_INITIALIZER
#endif

#define DECLARE_LOCK(NAME) \
    static pthread_mutex_t NAME = MUTEX_INIT;

#define LOCK(NAME) pthread_mutex_lock(&NAME)
#define UNLOCK(NAME) pthread_mutex_unlock(&NAME)

#else
#define DECLARE_LOCK(NAME)
#define LOCK(NAME)
#define UNLOCK(NAME)
#endif

int prom_process_common_init(void);
