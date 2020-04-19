// defines for libprom

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


#include <time.h>

#ifndef PROM_FILE
#include <stdio.h>

#define PROM_FILE FILE
#define PROM_PRINTF fprintf
#define PROM_GETS fgets
#define PROM_PUTS fputs
#define PROM_PUTC fputc
#endif

#ifdef NO_THREADS
#define _Atomic
#endif

// atomic double is just too gruesome
typedef _Atomic long long prom_value;

////////////////

enum prom_var_type { GAUGE, COUNTER };

struct prom_var {
    prom_value value;			// for simple vars
    enum prom_var_type type;
    const char *name;
    const char *help;
    double (*getter)(struct prom_var *);
    int (*format)(PROM_FILE *, struct prom_var *);
}
#ifdef __LP64__
    __attribute__((aligned(64)))
#endif
    ;

extern time_t prom_now;
#define STALE(T) ((T) == 0 || (prom_now - (T)) > 10)

int prom_format_simple(PROM_FILE *f, struct prom_var *pvp);
int prom_format_getter(PROM_FILE *f, struct prom_var *pvp);

// all prom_vars end up contiguous in a loader segment
#define PROM_SECTION_NAME prometheus

#define PROM_STR(NAME) PROM_STR2(NAME) 
#define PROM_STR2(NAME) #NAME
#define PROM_SECTION_STR PROM_STR(PROM_SECTION_NAME)

#ifdef __APPLE__
// Mach-O (OS X) wants section("segment,section")
#define PROM_SEGMENT "__DATA"
#define PROM_SECTION_PREFIX PROM_SEGMENT ","
#else
#define PROM_SECTION_PREFIX
#endif

#define PROM_SECTION_ATTR __attribute__((section (PROM_SECTION_PREFIX PROM_SECTION_STR)))

////////////////////////////////////////////////////////////////
// COUNTERs:

// mangle var NAME into struct name
// for internal use only
// users shouldn't touch prom_var innards
// (prevent decrement of counters and
// increment/set on non-simple vars)
#define _PROM_SIMPLE_COUNTER_NAME(NAME) PROM_SIMPLE_COUNTER_##NAME
#define _PROM_GETTER_COUNTER_NAME(NAME) PROM_GETTER_COUNTER_##NAME
#define _PROM_FORMAT_COUNTER_NAME(NAME) PROM_FORMAT_COUNTER_##NAME

#define PROM_GETTER_COUNTER_FN_NAME(NAME) NAME##_getter
#define PROM_FORMAT_COUNTER_FN_NAME(NAME) NAME##_format

// use to create functions!!
#define PROM_GETTER_COUNTER_FN_PROTO(NAME) static double PROM_GETTER_COUNTER_FN_NAME(NAME)(struct prom_var *pvp)
#define PROM_FORMAT_COUNTER_FN_PROTO(NAME) static int PROM_FORMAT_COUNTER_FN_NAME(NAME)(PROM_FILE *f, struct prom_var *pvp)

////////////////
// declare a simple counter
#define PROM_SIMPLE_COUNTER(NAME,HELP) \
    struct prom_var _PROM_SIMPLE_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ 0.0, COUNTER, #NAME, HELP, NULL, prom_format_simple }

// ONLY work on "simple" counters
#define PROM_SIMPLE_COUNTER_INC(NAME) _PROM_SIMPLE_COUNTER_NAME(NAME).value++
#define PROM_SIMPLE_COUNTER_INC_BY(NAME,BY) _PROM_SIMPLE_COUNTER_NAME(NAME).value += BY

////////////////
// declare counter with functio to fetch (non-decreasing) value
#define PROM_GETTER_COUNTER(NAME,HELP) \
    PROM_GETTER_COUNTER_FN_PROTO(NAME); \
    struct prom_var _PROM_GETTER_COUNTER_NAME(NAME) PROM_SECTION_ATTR =	\
	{ 0.0, COUNTER, #NAME, HELP, \
	  PROM_GETTER_COUNTER_FN_NAME(NAME), prom_format_getter }

////////////////
// declare a counter with a function to format names (typ. w/ labels)
// use PROM_FORMAT_COUNTER_FN_PROTO(NAME) { ...... } to declare
#define PROM_FORMAT_COUNTER(NAME,HELP) \
    PROM_FORMAT_COUNTER_FN_PROTO(NAME); \
    struct prom_var _PROM_FORMAT_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ 0.0, COUNTER, #NAME, HELP, NULL, PROM_FORMAT_COUNTER_FN_NAME(NAME) }

////////////////////////////////////////////////////////////////
// GAUGEs:

// mangle var NAME into struct name
// for internal use only
// users shouldn't touch prom_var innards
// (prevent increment/set on non-simple vars)
#define _PROM_SIMPLE_GAUGE_NAME(NAME) PROM_SIMPLE_GAUGE_##NAME
#define _PROM_GETTER_GAUGE_NAME(NAME) PROM_GETTER_GAUGE_##NAME
#define _PROM_FORMAT_GAUGE_NAME(NAME) PROM_FORMAT_GAUGE_##NAME

// use to create functions!!
#define PROM_GETTER_GAUGE_FN_NAME(NAME) NAME##_getter
#define PROM_FORMAT_GAUGE_FN_NAME(NAME) NAME##_format

#define PROM_GETTER_GAUGE_FN_PROTO(NAME) static double PROM_GETTER_GAUGE_FN_NAME(NAME)(struct prom_var *pvp)
#define PROM_FORMAT_GAUGE_FN_PROTO(NAME) static int PROM_FORMAT_GAUGE_FN_NAME(NAME)(PROM_FILE *f, struct prom_var *pvp)

////////////////
// declare a simple gauge
#define PROM_SIMPLE_GAUGE(NAME,HELP) \
    struct prom_var _PROM_SIMPLE_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ 0.0, GAUGE, #NAME, HELP, NULL, prom_format_simple }

// ONLY work on "simple" gauges
#define PROM_SIMPLE_GAUGE_INC(NAME) _PROM_SIMPLE_GAUGE_NAME(NAME).value++
#define PROM_SIMPLE_GAUGE_INC_BY(NAME,BY) _PROM_SIMPLE_GAUGE_NAME(NAME).value += BY
#define PROM_SIMPLE_GAUGE_SET(NAME, VAL) _PROM_SIMPLE_GAUGE_NAME(NAME) = VAL

////////////////
// declare gauge with functio to fetch (non-decreasing) value
#define PROM_GETTER_GAUGE(NAME,HELP) \
    PROM_GETTER_GAUGE_FN_PROTO(NAME); \
    struct prom_var _PROM_GETTER_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ 0.0, GAUGE, #NAME, HELP, \
	  PROM_GETTER_GAUGE_FN_NAME(NAME), prom_format_getter }

////////////////
// declare a gauge with a function to format names (typ. w/ labels)
// use PROM_FORMAT_GAUGE_FN_PROTO(NAME) { ...... } to declare
#define PROM_FORMAT_GAUGE(NAME,HELP) \
    PROM_FORMAT_GAUGE_FN_PROTO(NAME); \
    struct prom_var _PROM_FORMAT_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ 0.0, GAUGE, #NAME, HELP, NULL, PROM_FORMAT_GAUGE_FN_NAME(NAME) }

////////////////////////////////////////////////////////////////
// public interface:

extern const char *prom_namespace;	// must include trailing '_'

extern int prom_process_init(void);	// call to load process exporter
extern int prom_http_request(PROM_FILE *in, PROM_FILE *out, const char *who);
extern int prom_format_vars(PROM_FILE *f);

// helpers for formatters:
extern int prom_format_start(PROM_FILE *f, int *state, struct prom_var *pvp);
extern int prom_format_label(PROM_FILE *f, int *state, const char *name,
			     const char *format, ...)
    __attribute__ ((__format__ (__printf__, 4, 5)));
extern int prom_format_value(PROM_FILE *f, int *state, const char *format, ...)
    __attribute__ ((__format__ (__printf__, 3, 4)));
extern int prom_format_value_ll(PROM_FILE *f, int *state, long long value);
extern int prom_format_value_dbl(PROM_FILE *f, int *state, double value);
