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

#include "prom.h"

#define CONC(X,Y) CONC2(X,Y)
#define CONC2(X,Y) X##Y
#define START_PROM_SECTION CONC(__start_,PROM_SECTION_NAME)
#define STOP_PROM_SECTION CONC(__stop_,PROM_SECTION_NAME)

extern struct prom_var START_PROM_SECTION[1], STOP_PROM_SECTION[1];

// globals
time_t prom_now;

// prom_var.format for a simple variable
// avoid casting to double and f.p. formatting
// returns negative on failure
int
prom_format_simple(PROM_FILE *f, struct prom_var *pvp) {
    return PROM_PRINTF(f, "%s %lld\n", pvp->name, pvp->value);
}

// prom_var.format for a "getter" variable
// returns negative on failure
int
prom_format_getter(PROM_FILE *f, struct prom_var *pvp) {
    return PROM_PRINTF(f, "%s %.15g\n", pvp->name, pvp->getter(pvp));
}

static int
prom_format_one(PROM_FILE *f, struct prom_var *pvp) {
    switch (pvp->type) {
    case GAUGE:
	PROM_PRINTF(f, "# TYPE %s gauge\n", pvp->name);
	break;
    case COUNTER:
	PROM_PRINTF(f, "# TYPE %s counter\n", pvp->name);
	break;
    }
    PROM_PRINTF(f, "# HELP %s %s.\n", pvp->name, pvp->help);
    return (pvp->format)(f, pvp);
}

int
prom_format_vars(PROM_FILE *f) {
    struct prom_var *pvp;

    time(&prom_now);
    for (pvp = START_PROM_SECTION; pvp < STOP_PROM_SECTION; pvp++)
	prom_format_one(f, pvp);	// XXX check return?
    return 0;
}
