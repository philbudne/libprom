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

#include <stdlib.h>

#include "prom.h"
#include "common.h"

static double default_bins[] = {
    0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1, 2.5, 5, 10
};


static void
prom_histogram_check(struct prom_hist_var *phvp) {
    // SHOULD be per prom_hist_var lock!
    // (but this is quick, and should only get here on startup)
    DECLARE_LOCK(hist_check_lock);

    LOCK(hist_check_lock);
    if (!phvp->limits) {
	phvp->limits = default_bins;
	phvp->nbins = sizeof(default_bins)/sizeof(default_bins[0]);
    }
    if (!phvp->bins) {
	// XXX verify that limits are in sorted order?
	phvp->bins = calloc(phvp->nbins, sizeof(prom_value));
    }
    UNLOCK(hist_check_lock);
}

int
prom_histogram_observe(struct prom_hist_var *phvp, double value) {
    // SHOULD be per-prom_hist_var lock! (but only used for single add)
    DECLARE_LOCK(hist_observe_lock);
    int i;

    if (!phvp->bins)
	prom_histogram_check(phvp);

    // gcc can (in theory) have _Atomic double, but it's UGLY!
    LOCK(hist_observe_lock);	// SHOULD be per-histogram!
    phvp->sum += value;
    UNLOCK(hist_observe_lock);

    PROM_ATOMIC_INCREMENT(phvp->count, 1);

    i = phvp->nbins;
    while (--i >= 0 && value <= phvp->limits[i])
	PROM_ATOMIC_INCREMENT(phvp->bins[i], 1);
    return 0;
}

int
prom_format_histogram(PROM_FILE *f, struct prom_var *pvp) {
    struct prom_hist_var *phvp = (struct prom_hist_var *)pvp;
    int state, i;

    if (!phvp->bins)
	prom_histogram_check(phvp);

    // XXX taking per-histogram lock would guarantee
    // self-consistent data!
    // fetch count once? where???
    for (i = 0; i < phvp->nbins; i++) {
	prom_format_start(f, &state, pvp);
	PROM_PUTS("_bucket", f);
	prom_format_label(f, &state, "le", "%.15g", phvp->limits[i]);
	prom_format_value_pv(f, &state, phvp->bins[i]);
    }
    prom_format_start(f, &state, pvp);
    PROM_PUTS("_bucket", f);
    prom_format_label(f, &state, "le", "+Inf");
    prom_format_value_pv(f, &state, phvp->count);

    prom_format_start(f, &state, pvp);
    PROM_PUTS("_count", f);
    prom_format_value_pv(f, &state, phvp->count);

    prom_format_start(f, &state, pvp);
    PROM_PUTS("_sum", f);
    prom_format_value_dbl(f, &state, phvp->sum);

    return 0;				/* XXX */
}
