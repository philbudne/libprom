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

typedef long long prom_value;
#define PROM_ATOMIC_INCREMENT(VAR, BY) VAR += BY

#elif __cplusplus >= 201103L

#include <atomic>
typedef std::atomic<long long> prom_value;
#define PROM_ATOMIC_INCREMENT(VAR, BY) VAR += BY

#elif __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS__)

// support in gcc 4.6, clang 3.1
// available in glibc 2.28 (Ubuntu 18.10), FreeBSD 12, macOS Catalina
#include <stdatomic.h>			// C11 (optional feature)
typedef atomic_llong prom_value;
#define PROM_ATOMIC_INCREMENT(VAR, BY) VAR += BY

#else // not C11

// atomic_fetch_add_explicit(&atomic_counter, 1, memory_order_relaxed)
// on plain variable may suffice.

// _could_ make do with gcc 6.4:
// __atomic_fetch_add(&var, 1, __ATOMIC_SEQ_CST) ??

// works with gcc 4.1.2
typedef long long prom_value;
#define PROM_ATOMIC_INCREMENT(VAR, BY) \
    (void) __sync_add_and_fetch(&VAR, BY)

#endif // not C11

////////////////
#ifdef __cplusplus
// NOTE!  Requires g++ -std=c++17 (to allow initialization of atomic)
extern "C" {
#endif

enum prom_var_type {
    GAUGE,
    COUNTER,
    HISTOGRAM,
    LABEL
};

// empirical: works on both x86 and x86-64
// (look at doing fudgery when iterating?)
#define PROM_ALIGN __attribute__((aligned(32)))

struct prom_var {
    int size;
    enum prom_var_type type;
    const char *name;
    const char *help;
    int (*format)(PROM_FILE *, struct prom_var *);
} PROM_ALIGN;

struct prom_simple_var {
    struct prom_var base;
    prom_value value;
} PROM_ALIGN;

struct prom_getter_var {
    struct prom_var base;
    double (*getter)(void);
} PROM_ALIGN;

struct prom_hist_var {
    struct prom_var base;
    int nbins;			// not including +inf
    double *limits;		// double[nbins]
    prom_value *bins;
    prom_value count;		// also +Inf bin
    double sum;			// XXX need lock?
} PROM_ALIGN;

struct prom_labeled_var {
    struct prom_var base;
    const char *label;
} PROM_ALIGN;

// have prom_label_var sub/base class?
// would be needed to make list of all label vars
struct prom_simple_label_var {
    struct prom_var base;		// NOTE: name is label string!
    struct prom_labeled_var *parent_var; // variable being labeled
    // could have pointer to next label...
    prom_value value;
} PROM_ALIGN;

struct prom_getter_label_var {
    struct prom_var base;		// NOTE: name is label string!
    struct prom_labeled_var *parent_var; // variable being labeled
    // could have pointer to next label...
    double (*getter)(void);
} PROM_ALIGN;

extern time_t prom_now;			// set by prom_format_vars
#define STALE(T) ((T) == 0 || (prom_now - (T)) > 10)

int prom_format_simple(PROM_FILE *f, struct prom_var *pvp);
int prom_format_getter(PROM_FILE *f, struct prom_var *pvp);
int prom_format_histogram(PROM_FILE *f, struct prom_var *pvp);
int prom_format_labeled(PROM_FILE *f, struct prom_var *pvp);
int prom_format_simple_label(PROM_FILE *f, struct prom_var *pvp);
int prom_format_getter_label(PROM_FILE *f, struct prom_var *pvp);

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

#define PROM_SECTION_ATTR \
    __attribute__((section (PROM_SECTION_PREFIX PROM_SECTION_STR)))

////////////////////////////////////////////////////////////////
// COUNTERs:

// mangle var NAME into struct name
// for internal use only
// users shouldn't touch prom_var innards
// (prevent decrement of counters and increment on non-simple vars)
#define _PROM_SIMPLE_COUNTER_NAME(NAME) PROM_SIMPLE_COUNTER_##NAME
#define _PROM_GETTER_COUNTER_NAME(NAME) PROM_GETTER_COUNTER_##NAME
#define _PROM_FORMAT_COUNTER_NAME(NAME) PROM_FORMAT_COUNTER_##NAME
#define _PROM_LABELED_COUNTER_NAME(NAME) PROM_LABELED_COUNTER_##NAME
#define _PROM_SIMPLE_COUNTER_LABEL_NAME(NAME,LABEL) PROM_SIMPLE_COUNTER_##NAME##__LABEL__##LABEL
#define _PROM_GETTER_COUNTER_LABEL_NAME(NAME,LABEL) PROM_GETTER_COUNTER_##NAME##__LABEL__##LABEL

// avoids multiple declarations of same name with different type/class
// (cause ld error)
#define _PROM_NS(NAME) const char PROM_NS_##NAME = 1

// naming for functions:
#define PROM_GETTER_COUNTER_FN_NAME(NAME) NAME##_getter
#define PROM_FORMAT_COUNTER_FN_NAME(NAME) NAME##_format
#define PROM_GETTER_COUNTER_LABEL_FN_NAME(NAME,LABEL) NAME##_LABEL_##LABEL##_getter

// use to create functions!!
#define PROM_GETTER_COUNTER_FN_PROTO(NAME) \
    static double PROM_GETTER_COUNTER_FN_NAME(NAME)(void)
#define PROM_FORMAT_COUNTER_FN_PROTO(NAME) \
    static int PROM_FORMAT_COUNTER_FN_NAME(NAME)(PROM_FILE *f, \
						 struct prom_var *pvp)

#define PROM_GETTER_COUNTER_LABEL_FN_PROTO(NAME,LABEL) \
    static double PROM_GETTER_COUNTER_LABEL_FN_NAME(NAME,LABEL)(void)

////////////////
// declare a simple counter
#define PROM_SIMPLE_COUNTER(NAME,HELP) \
    _PROM_NS(NAME); \
    struct prom_simple_var _PROM_SIMPLE_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_simple_var), COUNTER, \
	  #NAME, HELP, prom_format_simple }, 0 }

// ONLY work on "simple" counters
#define PROM_SIMPLE_COUNTER_INC(NAME) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_COUNTER_NAME(NAME).value, 1)

#define PROM_SIMPLE_COUNTER_INC_BY(NAME,BY) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_COUNTER_NAME(NAME).value, BY)

////////////////
// declare counter with function to fetch (non-decreasing) value
#define PROM_GETTER_COUNTER(NAME,HELP) \
    _PROM_NS(NAME); \
    PROM_GETTER_COUNTER_FN_PROTO(NAME); \
    struct prom_getter_var _PROM_GETTER_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_getter_var), COUNTER, #NAME, HELP, \
	  prom_format_getter }, PROM_GETTER_COUNTER_FN_NAME(NAME) }

// declare var & function in one swell foop
#define PROM_GETTER_COUNTER_FN(NAME,HELP) \
    PROM_GETTER_COUNTER(NAME,HELP); \
    PROM_GETTER_COUNTER_FN_PROTO(NAME)

////////////////
// declare a counter with a function to format names (typ. w/ labels)
// use PROM_FORMAT_COUNTER_FN_PROTO(NAME) { ...... } to declare
#define PROM_FORMAT_COUNTER(NAME,HELP) \
    _PROM_NS(NAME); \
    PROM_FORMAT_COUNTER_FN_PROTO(NAME); \
    struct prom_var _PROM_FORMAT_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ sizeof(struct prom_var), COUNTER, \
	  #NAME, HELP, PROM_FORMAT_COUNTER_FN_NAME(NAME) }

// declare var & function:
#define PROM_FORMAT_COUNTER_FN(NAME,HELP) \
    PROM_FORMAT_COUNTER(NAME,HELP); \
    PROM_FORMAT_COUNTER_FN_PROTO(NAME)

////////////////
// declare counter with a single label name, and a static set of values.

// If output order needs to be tightly controlled
// could declare declare label subvars in a special segment, and have
// a (single) initializer function run thru the special segment,
// putting the labels into a linked list attached to the main var, and
// have the formatter for the main var iterate over the list,
// formatting the var/labels/values....

#define PROM_LABELED_COUNTER(NAME,LABEL,HELP) \
    _PROM_NS(NAME); \
    struct prom_labeled_var _PROM_LABELED_COUNTER_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_labeled_var), COUNTER, \
	  #NAME, HELP, prom_format_labeled }, LABEL }

////////
// declare a label on a PROM_LABELED_COUNTER with a "simple" value

#define PROM_SIMPLE_COUNTER_LABEL(NAME,LABEL_) \
    struct prom_simple_label_var _PROM_SIMPLE_COUNTER_LABEL_NAME(NAME,LABEL_) PROM_SECTION_ATTR =	\
	{ { sizeof(struct prom_simple_label_var), LABEL, \
	    #LABEL_, NULL, prom_format_simple_label }, &_PROM_LABELED_COUNTER_NAME(NAME), 0 }

// ONLY work on "simple" counters
#define PROM_SIMPLE_COUNTER_LABEL_INC(NAME,LABEL) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_COUNTER_LABEL_NAME(NAME,LABEL).value, 1)

#define PROM_SIMPLE_COUNTER_LABEL_INC_BY(NAME,LABEL,BY) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_COUNTER_LABEL_NAME(NAME,LABEL).value, BY)

////////
// declare a label on a PROM_LABELED_COUNTER with a "getter" value

#define PROM_GETTER_COUNTER_LABEL(NAME,LABEL_) \
    PROM_GETTER_COUNTER_LABEL_FN_PROTO(NAME,LABEL_); \
    struct prom_getter_label_var _PROM_GETTER_COUNTER_LABEL_NAME(NAME,LABEL_) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_getter_label_var), LABEL, \
	    #LABEL_, NULL, prom_format_getter_label }, \
	  &_PROM_LABELED_COUNTER_NAME(NAME), \
	  PROM_GETTER_COUNTER_LABEL_FN_NAME(NAME,LABEL_) }

// declare var & function in one line:
#define PROM_GETTER_COUNTER_LABEL_FN(NAME,LABEL_) \
    PROM_GETTER_COUNTER_LABEL(NAME,LABEL_); \
    PROM_GETTER_COUNTER_LABEL_FN_PROTO(NAME,LABEL_)

////////////////////////////////////////////////////////////////
// GAUGEs:

// mangle var NAME into struct name
// for internal use only
// users shouldn't touch prom_var innards
// (prevent increment/set on non-simple vars, decrement on counter)
// *BUT* allows multiple declaration of same metric name with different types!
#define _PROM_SIMPLE_GAUGE_NAME(NAME) PROM_SIMPLE_GAUGE_##NAME
#define _PROM_GETTER_GAUGE_NAME(NAME) PROM_GETTER_GAUGE_##NAME
#define _PROM_FORMAT_GAUGE_NAME(NAME) PROM_FORMAT_GAUGE_##NAME
#define _PROM_LABELED_GAUGE_NAME(NAME) PROM_LABELED_GAUGE_##NAME
#define _PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL) PROM_SIMPLE_GAUGE_##NAME##__LABEL__##LABEL
#define _PROM_GETTER_GAUGE_LABEL_NAME(NAME,LABEL) PROM_GETTER_GAUGE_##NAME##__LABEL__##LABEL

// use to create functions!!
#define PROM_GETTER_GAUGE_FN_NAME(NAME) NAME##_getter
#define PROM_FORMAT_GAUGE_FN_NAME(NAME) NAME##_format
#define PROM_GETTER_GAUGE_LABEL_FN_NAME(NAME,LABEL) NAME##_LABEL_##LABEL##_getter

#define PROM_GETTER_GAUGE_FN_PROTO(NAME) \
    static double PROM_GETTER_GAUGE_FN_NAME(NAME)(void)
#define PROM_FORMAT_GAUGE_FN_PROTO(NAME) \
    static int PROM_FORMAT_GAUGE_FN_NAME(NAME)(PROM_FILE *f, \
					       struct prom_var *pvp)

#define PROM_GETTER_GAUGE_LABEL_FN_PROTO(NAME,LABEL) \
    static double PROM_GETTER_GAUGE_LABEL_FN_NAME(NAME,LABEL)(void)

////////////////
// declare a simple gauge
#define PROM_SIMPLE_GAUGE(NAME,HELP) \
    _PROM_NS(NAME); \
    struct prom_simple_var _PROM_SIMPLE_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_simple_var), GAUGE, \
	    #NAME,HELP, prom_format_simple}, 0 }

// ONLY work on "simple" gauges
#define PROM_SIMPLE_GAUGE_INC(NAME) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_NAME(NAME).value, 1)

#define PROM_SIMPLE_GAUGE_INC_BY(NAME,BY) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_NAME(NAME).value, BY)

#define PROM_SIMPLE_GAUGE_DEC(NAME) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_NAME(NAME).value, -1)

#define PROM_SIMPLE_GAUGE_SET(NAME, VAL) \
    _PROM_SIMPLE_GAUGE_NAME(NAME).value = VAL

////////////////
// declare gauge with function to fetch (non-decreasing) value
#define PROM_GETTER_GAUGE(NAME,HELP) \
    _PROM_NS(NAME); \
    PROM_GETTER_GAUGE_FN_PROTO(NAME); \
    struct prom_getter_var _PROM_GETTER_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_getter_var), GAUGE, #NAME, HELP, \
	  prom_format_getter }, PROM_GETTER_GAUGE_FN_NAME(NAME) }

// declare var and function in one line:
#define PROM_GETTER_GAUGE_FN(NAME,HELP) \
    PROM_GETTER_GAUGE(NAME,HELP); \
    PROM_GETTER_GAUGE_FN_PROTO(NAME)

////////////////
// declare a gauge with a function to format names (typ. w/ labels)
// use PROM_FORMAT_GAUGE_FN_PROTO(NAME) { ...... } to declare
#define PROM_FORMAT_GAUGE(NAME,HELP) \
    _PROM_NS(NAME); \
    PROM_FORMAT_GAUGE_FN_PROTO(NAME); \
    struct prom_var _PROM_FORMAT_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ sizeof(struct prom_var), GAUGE, \
	  #NAME, HELP, PROM_FORMAT_GAUGE_FN_NAME(NAME) }

// declare var and function:
#define PROM_FORMAT_GAUGE_FN(NAME,HELP) \
    PROM_FORMAT_GAUGE(NAME,HELP); \
    PROM_FORMAT_GAUGE_FN_PROTO(NAME)

////////////////
// declare gauge with a single label name, and a static set of values.

#define PROM_LABELED_GAUGE(NAME,LABEL,HELP) \
    _PROM_NS(NAME); \
    struct prom_labeled_var _PROM_LABELED_GAUGE_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_labeled_var), GAUGE, \
	  #NAME, HELP, prom_format_labeled }, LABEL }

////////
// declare a label on a PROM_LABELED_GAUGE with a "simple" value

#define PROM_SIMPLE_GAUGE_LABEL(NAME,LABEL_) \
    struct prom_simple_label_var _PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL_) PROM_SECTION_ATTR =	\
	{ { sizeof(struct prom_simple_label_var), LABEL, \
	    #LABEL_, NULL, prom_format_simple_label }, &_PROM_LABELED_GAUGE_NAME(NAME), 0 }

// ONLY work on "simple" gauges
#define PROM_SIMPLE_GAUGE_LABEL_INC(NAME,LABEL) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL).value, 1)

#define PROM_SIMPLE_GAUGE_LABEL_INC_BY(NAME,LABEL,BY) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL).value, BY)

#define PROM_SIMPLE_GAUGE_LABEL_DEC(NAME,LABEL) \
    PROM_ATOMIC_INCREMENT(_PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL).value, -1)

#define PROM_SIMPLE_GAUGE_LABEL_SET(NAME,LABEL,VAL) \
    _PROM_SIMPLE_GAUGE_LABEL_NAME(NAME,LABEL).value = VAL

////////
// declare a label on a PROM_LABELED_GAUGE with a "getter" value

#define PROM_GETTER_GAUGE_LABEL(NAME,LABEL_) \
    PROM_GETTER_GAUGE_LABEL_FN_PROTO(NAME,LABEL_); \
    struct prom_getter_label_var _PROM_GETTER_GAUGE_LABEL_NAME(NAME,LABEL_) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_getter_label_var), LABEL, \
	    #LABEL_, NULL, prom_format_getter_label }, \
	  &_PROM_LABELED_GAUGE_NAME(NAME), \
	  PROM_GETTER_GAUGE_LABEL_FN_NAME(NAME,LABEL_) }

// declare var and function:
#define PROM_GETTER_GAUGE_LABEL_FN(NAME,LABEL_) \
    PROM_GETTER_GAUGE_LABEL(NAME,LABEL_); \
    PROM_GETTER_GAUGE_LABEL_FN_PROTO(NAME,LABEL_)

////////////////////////////////
// declare a histogram variable
#define _PROM_HISTOGRAM_NAME(NAME) PROM_HISTOGRAM_##NAME

// histogram with custom limits
#define PROM_HISTOGRAM_CUSTOM(NAME,HELP,LIMITS) \
    _PROM_NS(NAME); \
    struct prom_hist_var _PROM_HISTOGRAM_NAME(NAME) PROM_SECTION_ATTR = \
	{ {sizeof(struct prom_hist_var), HISTOGRAM, \
	   #NAME, HELP, prom_format_histogram }, \
	  sizeof(LIMITS)/sizeof(LIMITS[0]), LIMITS, NULL, 0, 0.0 }

// histogram with default limits
#define PROM_HISTOGRAM(NAME,HELP) \
    _PROM_NS(NAME); \
    struct prom_hist_var _PROM_HISTOGRAM_NAME(NAME) PROM_SECTION_ATTR = \
	{ { sizeof(struct prom_hist_var), HISTOGRAM, \
	  #NAME, HELP, prom_format_histogram }, \
	  0, NULL, NULL, 0, 0.0 }

extern int prom_histogram_observe(struct prom_hist_var *, double value);
#define PROM_HISTOGRAM_OBSERVE(NAME,VALUE) \
    prom_histogram_observe(&_PROM_HISTOGRAM_NAME(NAME), VALUE)

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
extern int prom_format_value_pv(PROM_FILE *f, int *state, prom_value value);
extern int prom_format_value_dbl(PROM_FILE *f, int *state, double value);

// network helpers
int prom_listen(int port, int family, int nonblock);
void prom_accept(int s);
int prom_dispatch(int s);
int prom_pool_init(int threads, const char *name);
void prom_http_interr(int s);
void prom_http_unavail(int s);

#ifndef PROM_DOUBLE_FORMAT
// not enough digits to print 2^63 (64-bit +inf)
// but avoids printing too many digits for microseconds???
#define PROM_DOUBLE_FORMAT "%.15g"
#endif

#ifdef __cplusplus
} // extern "C"
#endif
