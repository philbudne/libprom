// core/helper functions for 2label variables

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

// prom_var.format for a labeled var
// no value of its own
int
prom_format_2labeled(PROM_FILE *f, struct prom_var *pvp) {
    (void) f;
    (void) pvp;
    return 0;
}

// prom_var.format for a simple value label
// returns negative on failure
int
prom_format_simple_2label(PROM_FILE *f, struct prom_var *pvp) {
    struct prom_simple_2label_var *ps2lv = (struct prom_simple_2label_var *)pvp;
    struct prom_2labeled_var *parent = ps2lv->parent_var;
    int state;

    prom_format_start(f, &state, &parent->base);
    prom_format_label(f, &state, parent->label1, "%s", pvp->name);
    prom_format_label(f, &state, parent->label2, "%s", ps2lv->label2);
    return prom_format_value_pv(f, &state, ps2lv->value);
    return 0;
}

// prom_var.format for a getter value label
// returns negative on failure
int
prom_format_getter_2label(PROM_FILE *f, struct prom_var *pvp) {
    struct prom_getter_2label_var *pg2lv = (struct prom_getter_2label_var *)pvp;
    struct prom_2labeled_var *parent = pg2lv->parent_var;
    int state;

    prom_format_start(f, &state, &parent->base);
    prom_format_label(f, &state, parent->label1, "%s", pvp->name);
    prom_format_label(f, &state, parent->label2, "%s", pg2lv->label2);
    return prom_format_value_pv(f, &state, pg2lv->getter());
    return 0;
}

