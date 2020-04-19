// histogram helper

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

#include <malloc.h>

#include "prom.h"

static const double default_bins[] = {
    0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10
};

int
prom_histogram_observe(struct prom_var *pvp, double value) {
    int i;

    if (!pvp->limits) {
	pvp->limits = default_bins;
	pvp->nbins = sizeof(default_bins)/sizeof(default_bins[0]);
    }
    if (!pvp->bins) {
	pvp->bins = calloc(pvp->nbins + 1, sizeof(prom_value));
	// XXX check? zero???
    }

    // XXX wrap in (per histogram?) semaphore? _Atomic double is gruesome
    for (i = 0; i < pvp->nbins; i++) {
	// increment all bins with limits <= value
	pvp->bins[i]++;
	if (value <= pvp->limits[i])
	    return 0;
    }
    // here if above all limits; increment +inf bin
    pvp->bins[i]++;
    return 0;
}

int
prom_format_histogram(PROM_FILE *f, struct prom_var *pvp) {
    int state, i;
    for (i = 0; i < pvp->nbins; i++) {
	prom_format_start(f, &state, pvp);
	prom_format_label(f, &state, "le", "%.15g", pvp->limits[i]);
	prom_format_value_ll(f, &state, pvp->bins[i]);
    }
    prom_format_start(f, &state, pvp);
    prom_format_label(f, &state, "le", "+Inf");
    prom_format_value_ll(f, &state, pvp->bins[i]);
    return 0;				/* XXX */
}
