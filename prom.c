// core/helper functions for libprom

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

#include <stdarg.h>

#include "prom.h"

#ifdef __APPLE__
#include <mach-o/getsect.h>
#ifdef __LP64__
#define SECTION section_64
#else
#define SECTION section
#endif // APPLE, not LP64

#else // not __APPLE__

#define CONC(X,Y) CONC2(X,Y)
#define CONC2(X,Y) X##Y
#define START_PROM_SECTION CONC(__start_,PROM_SECTION_NAME)
#define STOP_PROM_SECTION CONC(__stop_,PROM_SECTION_NAME)

extern struct prom_var START_PROM_SECTION[1], STOP_PROM_SECTION[1];
#endif // not __APPLE__

// globals
time_t prom_now;
const char *prom_namespace = "";	// must include trailing '_'

int
prom_format_start(PROM_FILE *f, int *state, struct prom_var *pvp) {
    *state = 0;
    PROM_PUTS(prom_namespace, f);
    return PROM_PUTS(pvp->name, f);
}

int
prom_format_label(PROM_FILE *f, int *state, const char *name,
		  const char *format, ...) {
    va_list ap;
    char temp[32];

    va_start(ap, format);
    vsnprintf(temp, sizeof(temp), format, ap);
    va_end(ap);

    if (!*state) {
	PROM_PUTC('{', f);
	*state = 1;
    }
    else
	PROM_PUTC(',', f);

    PROM_PUTS(name, f);
    PROM_PUTC('=', f);
    PROM_PUTC('"', f);
    PROM_PUTS(temp, f);
    PROM_PUTC('"', f);

    return 0;				/* XXX */
}

int
prom_format_value(PROM_FILE *f, int *state, const char *format, ...) {
    va_list ap;
    char temp[32];

    va_start(ap, format);
    vsnprintf(temp, sizeof(temp), format, ap);
    va_end(ap);

    if (*state)
	PROM_PUTC('}', f);
    PROM_PUTC(' ', f);
    PROM_PUTS(temp, f);
    PROM_PUTC('\n', f);
    // XXX wack state??

    return 0;				/* XXX */
}

int
prom_format_value_ll(PROM_FILE *f, int *state, long long value) {
    return prom_format_value(f, state, "%lld", value);
}

int
prom_format_value_dbl(PROM_FILE *f, int *state, double value) {
    return prom_format_value(f, state, PROM_DOUBLE_FORMAT, value);
}

// prom_var.format for a simple variable
// avoid casting to double and f.p. formatting
// returns negative on failure
int
prom_format_simple(PROM_FILE *f, struct prom_var *pvp) {
    int state;
    struct prom_simple_var *psvp = (struct prom_simple_var *)pvp;

    prom_format_start(f, &state, pvp);
    return prom_format_value_ll(f, &state, psvp->value);
}

// prom_var.format for a "getter" variable
// returns negative on failure
int
prom_format_getter(PROM_FILE *f, struct prom_var *pvp) {
    int state;
    struct prom_getter_var *pgvp = (struct prom_getter_var *)pvp;

    prom_format_start(f, &state, pvp);
    return prom_format_value_dbl(f, &state, pgvp->getter());
}

static int
prom_format_one(PROM_FILE *f, struct prom_var *pvp) {
    switch (pvp->type) {
    case GAUGE:
	PROM_PRINTF(f, "# TYPE %s%s gauge\n", prom_namespace, pvp->name);
	break;
    case COUNTER:
	PROM_PRINTF(f, "# TYPE %s%s counter\n", prom_namespace, pvp->name);
	break;
#ifdef PROM_HISTOGRAMS
    case HISTOGRAM:
	PROM_PRINTF(f, "# TYPE %s%s histogram\n", prom_namespace, pvp->name);
	break;
#endif
    }
    PROM_PRINTF(f, "# HELP %s%s %s.\n", prom_namespace, pvp->name, pvp->help);
    return (pvp->format)(f, pvp);
}

int
prom_format_vars(PROM_FILE *f) {
#ifdef __APPLE__
    static struct prom_var *start_prom_section, *stop_prom_section;

    if (!start_prom_section) {
	const struct SECTION *sect = getsectbyname(PROM_SEGMENT, PROM_SECTION_STR);
	if (sect) {
	    start_prom_section = (struct prom_var *) sect->addr;
	    stop_prom_section  = (struct prom_var *) (sect->addr + sect->size);
	}
	else
	    return -1;
    }
#define START_PROM_SECTION start_prom_section
#define STOP_PROM_SECTION stop_prom_section
#endif
    struct prom_var *pvp;

    time(&prom_now);
    pvp = START_PROM_SECTION;
    while (pvp < STOP_PROM_SECTION) {
	prom_format_one(f, pvp);	// XXX check return?
	pvp = ((void *)pvp) + pvp->size;
    }
    return 0;
}
